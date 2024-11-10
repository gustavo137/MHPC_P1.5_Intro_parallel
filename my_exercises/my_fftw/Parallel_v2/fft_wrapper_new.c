#include "utilities.h"
#include <stdbool.h>
#include <string.h>
//
#include <complex.h>
#include <fftw3-mpi.h>
#include <fftw3.h>
#include <sys/time.h>
double seconds() {
  /* Return the second elapsed since Epoch (00:00:00 UTC, January 1, 1970) */
  struct timeval tmp;
  double sec;
  gettimeofday(&tmp, (struct timezone *)0);
  sec = tmp.tv_sec + ((double)tmp.tv_usec) / 1000000.0;
  return sec;
}
/*
 *  Index linearization is computed following row-major order.
 *  For more informtion see FFTW documentation:
 *  http://www.fftw.org/doc/Row_002dmajor-Format.html#Row_002dmajor-Format
 *
 */
int index_f(int i1, int i2, int i3, int n1, int n2, int n3) {
  return n3 * n2 * i1 + n3 * i2 + i3;
}
void init_fftw(fftw_dist_handler *fft, int n1, int n2, int n3,
               MPI_Comm mpi_comm) {
  int npes, rank;
  int buffer_size = 0;
  fft->mpi_comm = mpi_comm;
  /*
   *  Allocate a distributed grid for complex FFT using aligned memory
   * allocation See details here:
   *  http://www.fftw.org/fftw3_doc/Allocating-aligned-memory-in-Fortran.html#Allocating-aligned-memory-in-Fortran
   *  HINT: initialize all global and local dimensions. Consider the first
   * dimension being multiple of the number of processes
   *
   */
  // geting local sizes
  MPI_Comm_size(mpi_comm, &fft->npes);
  MPI_Comm_rank(mpi_comm, &fft->rank);
  if (((n1 % npes) || (n2 % npes)) && !rank) {
    fprintf(stdout, "\nN1 dimension must be multiple of the number of "
                    "processes. The program will be aborted...\n\n");
    MPI_Abort(mpi_comm, 1);
  }

  /* Fill the missing parts */
  fft->n1 = n1;
  fft->n2 = n2;
  fft->n3 = n3;
  fft->local_n1 = n1 / fft->npes;
  fft->local_n1_offset = fft->rank * fft->local_n1;
  fft->global_size_grid = n1 * n2 * n3;
  fft->local_size_grid = fft->local_n1 * n2 * n3;

  /*
   * Allocate fft->fftw_data and create an FFTW plan for each 1D FFT among all
   * dimensions
   *
   */
  // I will use plan_2d and plan_1d
  // plan to y and z axis
  fft->fw_plan_2d =
      fftw_plan_dft_2d(n2, n3, NULL, NULL, FFTW_FORWARD, FFTW_MEASURE);
  fft->bw_plan_2d =
      fftw_plan_dft_2d(n2, n3, NULL, NULL, FFTW_BACKWARD, FFTW_MEASURE);
  // Now a plan for x axis
  fft->fw_plan_1d =
      fftw_plan_dft_1d(n1, NULL, NULL, FFTW_FORWARD, FFTW_MEASURE);
  fft->bw_plan_1d =
      fftw_plan_dft_1d(n1, NULL, NULL, FFTW_BACKWARD, FFTW_MEASURE);
}
void close_fftw(fftw_dist_handler *fft) {
  fftw_destroy_plan(fft->bw_plan_1d);
  fftw_destroy_plan(fft->bw_plan_2d);

  fftw_destroy_plan(fft->fw_plan_1d);
  fftw_destroy_plan(fft->fw_plan_2d);
  fftw_free( fft->fftw_data );
}
/* This subroutine uses fftw to calculate 3-dimensional discrete FFTs.
 * The data in direct space is assumed to be real-valued
 * The data in reciprocal space is complex.
 * direct_to_reciprocal indicates in which direction the FFT is to be calculated
 *
 * Note that for real data in direct space (like here), we have
 * F(N-j) = conj(F(j)) where F is the array in reciprocal space.
 * Here, we do not make use of this property.
 * Also, we do not use the special (time-saving) routines of FFTW which
 * allow one to save time and memory for such real-to-complex transforms.
 *
 * f: array in direct space
 * F: array in reciprocal space
 *
 * F(k) = \sum_{l=0}^{N-1} exp(- 2 \pi I k*l/N) f(l)
 * f(l) = 1/N \sum_{k=0}^{N-1} exp(+ 2 \pi I k*l/N) F(k)
 *
 */

// form 1
///////////////////////////////////////////////////////////////////////////////////////////////////
void fft_3d(fftw_dist_handler *fft, double *data_direct, fftw_complex *data_rec,
            bool direct_to_reciprocal) {
  double fac;
  int i1, i2, i3, index, start_index, end_index, index_buf, i2_loc;
  int n2 = fft->n2, n3 = fft->n3, n1 = fft->n1, npes, block_dim, nblock;

  /* Allocate buffers to send and receive data */
  fftw_complex *local_data = fftw_malloc(sizeof(fftw_complex)*fft->local_size_grid);
  fftw_complex *transposed_data = fftw_malloc(sizeof(fftw_complex)*fft->local_size_grid);
  // check for good allocation 
  if(!local_data || !transposed_data){
     fprintf(stderr,"Error allocatin memory in wrapper fft_3d.\n");
     MPI_Abort(fft->mpi_comm,1);
  }

  MPI_Comm_size(fft->mpi_comm, &npes);

  // Now distinguish in which direction the FFT is performed
  if(direct_to_reciprocal){//forward FFT
    // data real to complex
    for(ptrdiff_t i =0;i< fft->local_size_grid;++i){
      local_data[i]=data_direct[i]+0.0*I;
    }
    // now do the FFT along y and z for each x
    for (ptrdiff_t i1_local = 0; i1_local < fft->local_n1; i1_local++){
      fftw_execute_dft(fft->fw_plan_2d,&local_data[i1_local*fft->n2*fft->n3],&local_data[i1_local*fft->n2*fft->n3]);
    }
    ///// transpose the data 
     // get somethin as transposed_data
    transpose_data(fft,local_data,transposed_data);
    //// do the FFT along x 
    for (ptrdiff_t i = 0; i < fft->local_size_grid; i++){
      fftw_execute_dft(fft->fw_plan_1d,&transposed_data[i],&transposed_data[i]);
    }
    // last step put the data in data_rec
    memcpy(data_rec,transposed_data,sizeof(fftw_complex)*fft->local_size_grid);
  } else {// backward FFT
    memcpy(transposed_data,data_rec,sizeof(fftw_complex)*fft->local_size_grid);

    // do the inverse FFT along x
    for (ptrdiff_t i2 = 0; i2 < fft->n2; ++i2){
      for (ptrdiff_t i3 = 0; i3 < fft->n3; ++i3){
        ptrdiff_t idx = (i2*fft->n3 +i3)*fft->n1;
        fftw_execute_dft(fft->bw_plan_1d,&transposed_data[idx],&transposed_data[idx]);
      }
    }
    //transpose the data back to the original distribution
     transpose_data(fft,transposed_data,local_data);
    //////////////////////////////////
    // perform the inverse FFT along y and z for each x 
    for(ptrdiff_t i1_local = 0;i1_local<fft->local_n1;++i1_local){
      fftw_execute_dft(fft->bw_plan_2d,&local_data[i1_local*fft->n2*fft->n3],&local_data[i1_local*fft->n2*fft->n3]);
    }
    //Extract the real part and scale the data using fac = 1/(n1*n2*n3)
    fac = 1.0/(fft->n1*fft->n2*fft->n3);
    for (ptrdiff_t i = 0; i < fft->local_size_grid; i++){
      data_direct[i]=creal(local_data[i])*fac;
    }
    
  }
  fftw_free(local_data);
  fftw_free(transposed_data);
}  

// this function make the transposition and the alltoall 
void transpose_data(){

}
  ////////////////////////////////////////////////////////////////////////////////////////////
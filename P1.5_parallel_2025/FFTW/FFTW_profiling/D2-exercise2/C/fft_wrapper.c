/* Assignement:
 * Here you have to modify the includes, the array sizes and the fftw calls, to
 * use the fftw-mpi
 *
 * Regarding the fftw calls. here is the substitution
 * fftw_plan_dft_3d -> fftw_mpi_plan_dft_3d
 * ftw_execute_dft  > fftw_mpi_execute_dft
 * use fftw_mpi_local_size_3d for local size of the arrays
 *
 * Created by G.P. Brandino, I. Girotto, R. Gebauer
 * Last revision: March 2016
 *
 */
#include "utilities.h"
#include <complex.h>
#include <fftw3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

void init_fftw(fftw_dist_handler *fft, int n1, int n2, int n3, MPI_Comm comm) {

  int npes, mype;
  int buffer_size = 0;

  fft->mpi_comm = comm;

  /*
   *  Allocate a distributed grid for complex FFT using aligned memory
   * allocation See details here:
   *  http://www.fftw.org/fftw3_doc/Allocating-aligned-memory-in-Fortran.html#Allocating-aligned-memory-in-Fortran
   *  HINT: initialize all global and local dimensions. Consider the first
   * dimension being multiple of the number of processes
   *
   */

  MPI_Comm_size(comm, &npes);
  MPI_Comm_rank(comm, &mype);

  if (((n1 % npes) || (n2 % npes)) && !mype) {

    fprintf(stdout, "\nN1 dimension must be multiple of the number of "
                    "processes. The program will be aborted...\n\n");
    MPI_Abort(comm, 1);
  }

  /* Fill the missing parts */
  fft->n1 = n1;
  fft->n2 = n2;
  fft->n3 = n3;
  fft->local_n1 = n1 / npes;
  fft->local_n2 = n2 / npes;
  fft->local_n1_offset = mype * (fft->local_n1);
  fft->global_size_grid = n1 * n2 * n3;
  fft->local_size_grid = (fft->local_n1) * n2 * n3;

  /*
   * Allocate fft->fftw_data and create an FFTW plan for each 1D and 2d DFT
   *
   */


  
  fft->fftw_data =
      (fftw_complex *)fftw_malloc(fft->local_n1 * n2 * n3 * sizeof(fftw_complex));

  fft->fw_plan_1d =
      fftw_plan_dft_1d(fft->local_n1 * npes, fft->fftw_data, fft->fftw_data,
                       FFTW_FORWARD, FFTW_ESTIMATE);
  fft->fw_plan_2d = fftw_plan_dft_2d(n2, n3, fft->fftw_data, fft->fftw_data,
                                     FFTW_FORWARD, FFTW_ESTIMATE);

  fft->bw_plan_1d =
      fftw_plan_dft_1d(fft->local_n1 * npes, fft->fftw_data, fft->fftw_data,
                       FFTW_BACKWARD, FFTW_ESTIMATE);
  fft->bw_plan_2d = fftw_plan_dft_2d(n2, n3, fft->fftw_data, fft->fftw_data,
                                     FFTW_BACKWARD, FFTW_ESTIMATE);


  /* int n[] = {fft->local_n1}; */
  /* fft->fw_multplan_1d = (1, n, fft->n1*fft->n2, */
  /*                        fft->fftw_data, n, */
  /*                         ); */                                
}

void close_fftw(fftw_dist_handler *fft) {

  fftw_destroy_plan(fft->bw_plan_1d);
  fftw_destroy_plan(fft->bw_plan_2d);

  fftw_destroy_plan(fft->fw_plan_1d);
  fftw_destroy_plan(fft->fw_plan_2d);

  fftw_free(fft->fftw_data);
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

void fft_3d(fftw_dist_handler *fft, double *data_direct, fftw_complex *data_rec,
            bool direct_to_reciprocal) {

  double fac;
  int i1, i2, i3, index, start_index, end_index, index_buf, i2_loc;
  int n2 = fft->n2, n3 = fft->n3, n1 = fft->n1, npes, block_dim, nblock;
  int local_n1 = fft->local_n1;
  int local_n2 = fft->local_n2;
  fftw_complex *resrvbuf;

  //query the number of processes
  MPI_Comm_size(fft->mpi_comm, &npes);
  /* Allocate buffers to send and receive data */
  resrvbuf = (fftw_complex *)fftw_malloc(local_n1 * n2 * n3 * sizeof(fftw_complex));

  // Now distinguish in which direction the FFT is performed
  if (direct_to_reciprocal) {
    for (i1 = 0; i1 < local_n1; i1++) {
      for (i2 = 0; i2 < n2; i2++) {
        for (i3 = 0; i3 < n3; i3++) {
          // initialize the data in the structure
          fft->fftw_data[n3 * n2 * i1 + n3 * i2 + i3] =
              data_direct[n3 * n2 * i1 + n3 * i2 + i3] + 0.0 * I;
        }
      }
      // execute a 2d DFT on every slice. The beginning of every slice is at i2=0, 13=0
      fftw_execute_dft(fft->fw_plan_2d, &(fft->fftw_data[n3 * n2 * i1]),
                       &(fft->fftw_data[n3 * n2 * i1]));
    }

    /*
     * Reorder the different my data blocks in preapration for MPI_Alltoall.
     * Put the reordered data in resrvbuf. Look at your drawing to visualize 
     * and understand this process better.
    */
    for (int block = 0; block < npes; block++) {
      for (i1 = 0; i1 < local_n1; i1++) {
        for (i2 = 0; i2 < local_n2; i2++) {
          for (i3 = 0; i3 < n3; i3++) {
            // source index
            int src_i1 = i1;
            int src_i2 = i2 + block * local_n2;
            int src_index = src_i1 * (n2 * n3) + src_i2 * n3 + i3;

            // destination index
            int dest_i1 = i1 + block * local_n1;
            int dest_i2 = i2;
            int dest_index = dest_i1 * (local_n2 * n3) + dest_i2 * n3 + i3;

            resrvbuf[dest_index] = fft->fftw_data[src_index];
          }
        }
      }
    }

    /**
     * Perform an Alltoall communication to perform a blocked transposition.
     * Use fft->fftw_data as a receiving buffer. 
    */

    MPI_Alltoall(resrvbuf,local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE ,fft->fftw_data, local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE, fft->mpi_comm);

    /**
     * Perform a local transpose in i1 <-----> i3 directions in my
     * fft->fftw_data. Put transpose data in the resrv buffer. At this point,
     * fftw_data is (local_n1 * npes) by local_n2 by n3
    */

    for (i1 = 0; i1 < local_n1 * npes; i1++) {
      for (i2 = 0; i2 < local_n2; i2++) {
        for (i3 = 0; i3 < n3; i3++) {
          // orginal index
          int orig_index = i1 * (local_n2 * n3) + i2 * n3 + i3;
          // transposed index
          int trans_index =
              i3 * (local_n2 * local_n1 * npes) + i2 * (local_n1 * npes) + i1;
          // set the transposed element
          resrvbuf[trans_index] = fft->fftw_data[orig_index];
        }
      }
    }

    // perform a 1d dft along the i3 direction of resrvbuf. resrvbuf is n3 by
    // local_n2 by (local_n1 * npes)
    for (i1 = 0; i1 < n3; i1++) {
      for (i2 = 0; i2 < local_n2; i2++) {
        int the_index = i1 * (local_n2 * local_n1 * npes) +
                        i2 * (local_n1 * npes); // just set i3=0
        fftw_execute_dft(fft->fw_plan_1d, &(resrvbuf[the_index]),
                         &(resrvbuf[the_index]));
      }
    }

    //<<----------------lets us now retrace our step

    /**
     * Perform a local transpose in i1 <-----> i3 directions in resrvbuf. Put
     * transposed data in the fft->fftw_data buffer. At this point, resrvbuf is
     * n3 by local_n2 by (local_n1 * npes)
     */

    for (i1 = 0; i1 < n3; i1++) {
      for (i2 = 0; i2 < local_n2; i2++) {
        for (i3 = 0; i3 < local_n1 * npes; i3++) {
          int orig_index = i1 * (local_n2 * (local_n1 * npes)) +
                           i2 * ((local_n1 * npes)) + i3;
          int trans_index = i3 * (local_n2 * n3) + i2 * n3 + i1;
          fft->fftw_data[trans_index] = resrvbuf[orig_index];
        }
      }
    }

    // make another Alltoall communication to perform blocked transposition.
    // Send fft->fftw_data and receive on resrvbuf
    MPI_Alltoall(fft->fftw_data, local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE,resrvbuf,local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE,fft->mpi_comm);

    /// perform the reordering again. Write the reordered data to
    /// fft->fftw_data
    for (int block = 0; block < npes; block++) {
      for (i1 = 0; i1 < local_n1; i1++) {
        for (i2 = 0; i2 < local_n2; i2++) {
          for (i3 = 0; i3 < n3; i3++) {
            // source index
            int src_i1 = i1 + block * local_n1;
            int src_i2 = i2;
            int src_index = src_i1 * (local_n2 * n3) + src_i2 * n3 + i3;

            // destination index
            int dest_i1 = i1;
            int dest_i2 = i2 + block * local_n2;
            int dest_index = dest_i1 * (n2 * n3) + dest_i2 * n3 + i3;

            fft->fftw_data[dest_index] = resrvbuf[src_index];
          }
        }
      }
    }


    //<<---------fftw_data now contains the full 3d DFT of the data in the
    //buffer pointed to by data_direc. Copy the contents of  fft->fftw_data
    //to data_rec.
    memcpy(data_rec,fft->fftw_data,fft->local_size_grid*sizeof(fftw_complex));

    free(resrvbuf);

  } else { 
    /**
     * We implement the inverse 3D DFT. The steps are identical save for the 
     * fft->bw_plan_1d and fft->bw_plan_1d that we execute here. 
     */
    
    for (i1 = 0; i1 < local_n1; i1++) {
      for (i2 = 0; i2 < n2; i2++) {
        for (i3 = 0; i3 < n3; i3++) {
          // initialize the data in the structure
          fft->fftw_data[n3 * n2 * i1 + n3 * i2 + i3] = data_rec[n3 * n2 * i1 + n3 * i2 + i3];
        }
      }
      // execute a 2d DFT on every slice. The beginning of every slice is at i2=0, 13=0
      fftw_execute_dft(fft->bw_plan_2d, &(fft->fftw_data[n3 * n2 * i1]),&(fft->fftw_data[n3 * n2 * i1]));
    }

    /*
     * Reorder the different my data blocks in preapration for MPI_Alltoall.
     * Put the reordered data in resrvbuf. Look at your drawing to visualize 
     * and understand this process better.
    */
    for (int block = 0; block < npes; block++) {
      for (i1 = 0; i1 < local_n1; i1++) {
        for (i2 = 0; i2 < local_n2; i2++) {
          for (i3 = 0; i3 < n3; i3++) {
            // source index
            int src_i1 = i1;
            int src_i2 = i2 + block * local_n2;
            int src_index = src_i1 * (n2 * n3) + src_i2 * n3 + i3;

            // destination index
            int dest_i1 = i1 + block * local_n1;
            int dest_i2 = i2;
            int dest_index = dest_i1 * (local_n2 * n3) + dest_i2 * n3 + i3;

            resrvbuf[dest_index] = fft->fftw_data[src_index];
          }
        }
      }
    }

    /**
     * Perform an Alltoall communication to perform a blocked transposition.
     * Use fft->fftw_data as a receiving buffer. 
    */

    MPI_Alltoall(resrvbuf,local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE ,fft->fftw_data, local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE, fft->mpi_comm);

    /**
     * Perform a local transpose in i1 <-----> i3 directions in my
     * fft->fftw_data. Put transpose data in the resrv buffer. At this point,
     * fftw_data is (local_n1 * npes) by local_n2 by n3
    */
    for (i1 = 0; i1 < local_n1 * npes; i1++) {
      for (i2 = 0; i2 < local_n2; i2++) {
        for (i3 = 0; i3 < n3; i3++) {
          // orginal index
          int orig_index = i1 * (local_n2 * n3) + i2 * n3 + i3;
          // transposed index
          int trans_index =
              i3 * (local_n2 * local_n1 * npes) + i2 * (local_n1 * npes) + i1;
          // set the transposed element
          resrvbuf[trans_index] = fft->fftw_data[orig_index];
        }
      }
    }

    // perform a 1d dft along the i3 direction of resrvbuf. resrvbuf is n3 by
    // local_n2 by (local_n1 * npes)
    for (i1 = 0; i1 < n3; i1++) {
      for (i2 = 0; i2 < local_n2; i2++) {
        int the_index = i1 * (local_n2 * local_n1 * npes) +
                        i2 * (local_n1 * npes); // just set i3=0
        fftw_execute_dft(fft->bw_plan_1d, &(resrvbuf[the_index]),&(resrvbuf[the_index]));
      }
    }

    //<<----------------lets us now retrace our step

    /**
     * Perform a local transpose in i1 <-----> i3 directions in resrvbuf. Put
     * transposed data in the fft->fftw_data buffer. At this point, resrvbuf is
     * n3 by local_n2 by (local_n1 * npes)
     */

    for (i1 = 0; i1 < n3; i1++) {
      for (i2 = 0; i2 < local_n2; i2++) {
        for (i3 = 0; i3 < local_n1 * npes; i3++) {
          int orig_index = i1 * (local_n2 * (local_n1 * npes)) +
                           i2 * ((local_n1 * npes)) + i3;
          int trans_index = i3 * (local_n2 * n3) + i2 * n3 + i1;
          fft->fftw_data[trans_index] = resrvbuf[orig_index];
        }
      }
    }

    // make another Alltoall communication to perform blocked transposition.
    // Send fft->fftw_data and receive on resrvbuf
    MPI_Alltoall(fft->fftw_data,local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE,resrvbuf,local_n1 * local_n2 * n3 * sizeof(fftw_complex), MPI_BYTE,fft->mpi_comm);

    /// perform the reordering again. Write the reordered data to fft->
    /// fft->fftw_data
    for (int block = 0; block < npes; block++) {
      for (i1 = 0; i1 < local_n1; i1++) {
        for (i2 = 0; i2 < local_n2; i2++) {
          for (i3 = 0; i3 < n3; i3++) {
            // source index
            int src_i1 = i1 + block * local_n1;
            int src_i2 = i2;
            int src_index = src_i1 * (local_n2 * n3) + src_i2 * n3 + i3;

            // destination index
            int dest_i1 = i1;
            int dest_i2 = i2 + block * local_n2;
            int dest_index = dest_i1 * (n2 * n3) + dest_i2 * n3 + i3;
            
            fft->fftw_data[dest_index] = resrvbuf[src_index];
          }
        }
      }
    }

    fac = 1.0 / (n1 * n2 * n3);
    
    for (int i = 0; i < fft->local_size_grid; ++i) {
      data_direct[i] = creal(fft->fftw_data[i]) * fac;
    }
    
  }
  
}

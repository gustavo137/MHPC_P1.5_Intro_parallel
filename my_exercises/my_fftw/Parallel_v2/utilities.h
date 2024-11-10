/* 
 * Created by G.P. Brandino, I. Girotto, R. Gebauer
 * Last revision: March 2016
 */

#ifndef _FFTW_UTLITIES_
#define _FFTW_UTLITIES_
#include <complex.h>
#include <sys/time.h>
#include <fftw3.h>
#include <stdbool.h>
#include <mpi.h>
#define pi 3.14159265358979323846


typedef struct {
  fftw_plan fw_plan_2d; 
  fftw_plan bw_plan_2d;
  fftw_plan fw_plan_1d;
  fftw_plan bw_plan_1d;
  fftw_complex *fftw_data;
  ptrdiff_t n1,n2,n3;
  int mype, npes;
  ptrdiff_t global_size_grid; /* Global size of the FFT grid */
  ptrdiff_t local_size_grid;  /* Local size of the FFT grid */
  ptrdiff_t local_n1;         /* Local dimension of n1 */
  ptrdiff_t local_n2;
  ptrdiff_t local_n1_offset;  /* Offset due to possible rests */  
  MPI_Comm mpi_comm;
} fftw_dist_handler;


double seconds();
inline int index_f ( int i1, int i2, int i3, int n1, int n2, int n3 );
//inline int index_f ( int i1, int i2, int i3, int n2, int n3 );
//
void plot_data_1d_parallel( char* name, int n1, int n2, int n3, int n1_local, int  n1_local_offset, int dir, double* data);
void plot_data_2d_parallel( char* name, int n1, int n2, int n3, int n1_local, int  n1_local_offset, int dir, double* data);

//
void plot_data_1d( char* name, int n1, int n2, int n3, int dir, double* data );
void plot_data_2d( char* name, int n1, int n2, int n3, int dir, double* data );
void init_fftw( fftw_dist_handler* fft, int n1, int n2, int n3, MPI_Comm mpi_comm );
void close_fftw( fftw_dist_handler* fft );

void derivative( fftw_dist_handler* fft,int n1, int n2, int n3, double L1, double L2, double L3, int ipol, double* data, double* deriv );

/* New interface for fft_3d which includes a parameter of kind fftw_mpi_handler */
//void fft_3d( fftw_mpi_handler* fft, int n1, int n2, int n3, double *data_direct, fftw_complex* data_rec, bool direct_to_reciprocal );
//function transpose_data, fft_3d uses this function 
void fft_3d( fftw_dist_handler* fft, double *data_direct, fftw_complex* data_rec, bool direct_to_reciprocal );
void transpose_data(fftw_dist_handler *fft, fftw_complex *local_data, fftw_complex *transposed_data);
#endif

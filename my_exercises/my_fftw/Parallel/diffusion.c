/* Assignment: 
 * Parallelize the code, using fftw-mpi
 * This amount to 
 *   - distribute the data contained in diffusivity, conc, dconc, aux1, aux2 in the way fftw-mpi expects
 *   - modify the fftw calls in fftw-mpi in p_fftw_wrapper
 * You will need to modify the files
 *   - diffusion.c   
 *   - derivative.c
 *   - fftw_wrapper.c
 *
 * Created by G.P. Brandino, I. Girotto, R. Gebauer
 * Last revision: March 2016
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <mpi.h>
#include <fftw3-mpi.h>
#include "utilities.h"
// http://www.fftw.org/doc/MPI-Files-and-Data-Types.html#MPI-Files-and-Data-Types

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

int main(int argc, char** argv){
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Dimensions of the system
  double L1 = 10.0, L2 = 10.0, L3 = 20.0;
  // Grid size
  int n1 = 48, n2 = 48, n3 = 96;
  // time step for time integration
  double dt = 2.e-3; 
  // number of time steps
  int nstep = 101; 
  // Radius of diffusion channel
  double rad_diff = 0.7;
  // Radius of starting concentration
  double rad_conc = 0.6;
  double start, end;

  double *diffusivity, *conc, *dconc, *aux1, *aux2;
  
  int i1, i2, i3, ipol, istep;
  int i1_local, i, index;

  double f1conc, f2conc, f3conc, f1diff, f2diff, f3diff, fac, ss;
  double x1, x2 , x3, rr, r2mean;
  double ss_local, r2mean_local;
  fftw_mpi_handler fft_h;
  // Initialize the fftw system and local dimension
  init_fftw( &fft_h, n1, n2, n3, MPI_COMM_WORLD);
  ptrdiff_t local_n1 = fft_h.local_n1;
  ptrdiff_t local_n1_offset = fft_h.local_n1_offset;
  ptrdiff_t local_size_grid = fft_h.local_size_grid;
  // Allocate distributed memory arrays
  diffusivity = (double*)malloc(sizeof(double) * local_size_grid);
  conc = (double*)malloc(sizeof(double) * local_size_grid);
  dconc = (double*)malloc(sizeof(double) * local_size_grid);
  aux1 = (double*)malloc(sizeof(double) * local_size_grid);
  aux2 = (double*)malloc(sizeof(double) * local_size_grid);

  // Check for successful allocation
  if (!diffusivity || !conc || !dconc || !aux1 || !aux2) {
    fprintf(stderr, "Memory allocation failed.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  // Define the diffusivity inside the system and the starting concentration
  ss = 0.0;   // ss is to integrate (and normalize) the concentration

  for (i3 = 0; i3 < n3; ++i3) {
    x3 = L3 * i3 / n3;
    f3diff = exp(-((x3 - 0.5 * L3) / rad_diff) * ((x3 - 0.5 * L3) / rad_diff));
    f3conc = exp(-((x3 - 0.5 * L3) / rad_conc) * ((x3 - 0.5 * L3) / rad_conc));
    
    for (i2 = 0; i2 < n2; ++i2) {
      x2 = L2 * i2 / n2;
      f2diff = exp(-((x2 - 0.5 * L2) / rad_diff) * ((x2 - 0.5 * L2) / rad_diff));
      f2conc = exp(-((x2 - 0.5 * L2) / rad_conc) * ((x2 - 0.5 * L2) / rad_conc));
      
      for (i1_local = 0; i1_local < local_n1; ++i1_local) {
          i1 = i1_local + local_n1_offset;
          x1 = L1 * i1 / n1;
          f1diff = exp(-((x1 - 0.5 * L1) / rad_diff) * ((x1 - 0.5 * L1) / rad_diff));
          f1conc = exp(-((x1 - 0.5 * L1) / rad_conc) * ((x1 - 0.5 * L1) / rad_conc));
          index = index_f(i1_local, i2, i3, n2, n3);
          diffusivity[index] = MAX(f1diff * f2diff, f2diff * f3diff);
          conc[index] = f1conc * f2conc * f3conc;
          ss += conc[index];
      }
    }
  }
  
  // Output routines
  plot_data_2d_parallel("diffusivity", n1, n2, n3, local_n1, local_n1_offset, 1, diffusivity);
  plot_data_2d_parallel("diffusivity", n1, n2, n3, local_n1, local_n1_offset, 2, diffusivity);
  plot_data_2d_parallel("diffusivity", n1, n2, n3, local_n1, local_n1_offset, 3, diffusivity);
  
  fac = L1 * L2 * L3 / (n1 * n2 * n3);
  
  // Normalize the concentration
  double ss_global;
  MPI_Allreduce(&ss, &ss_global, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  ss = 1.0 / (ss_global * fac);
  
  for (i = 0; i < local_size_grid; ++i) {
    conc[i] *= ss;
  }

  /*
    * Now everything is defined: system size, diffusivity inside the system, and
    * the starting concentration
    *
    * Start the dynamics
    *
    */
  start = seconds();
  for (istep = 1; istep <= nstep; ++istep){
    // Initialize dconc
    for (i = 0; i < local_size_grid; ++i){
      dconc[i] = 0.0;
    }

    // Compute derivatives
    for (ipol = 1; ipol <= 3; ++ipol) {
      derivative(&fft_h, n1, n2, n3, L1, L2, L3, ipol, conc, aux1);
      
      for (i = 0; i < local_size_grid; ++i) {
        aux1[i] *= diffusivity[i];
      }

      derivative(&fft_h, n1, n2, n3, L1, L2, L3, ipol, aux1, aux2);
      
      // Summing up contributions
      for (i = 0; i < local_size_grid; ++i) {
        dconc[i] += aux2[i];
      }
    }
    
    for (i = 0; i < local_size_grid; ++i) {
      conc[i] += dt * dconc[i];
    }


    if (istep%30 == 1){
      ss_local = 0.0;
      r2mean_local = 0.0;

      for (i3 = 0; i3 < n3; ++i3) {
        x3 = L3 * i3 / n3 - 0.5 * L3;
        
        for (i2 = 0; i2 < n2; ++i2) {
          x2 = L2 * i2 / n2 - 0.5 * L2;
          
          for (i1_local = 0; i1_local < local_n1; ++i1_local) {
            i1 = i1_local + local_n1_offset;
            x1 = L1 * i1 / n1 - 0.5 * L1;
            rr = x1 * x1 + x2 * x2 + x3 * x3;
            index = index_f(i1_local, i2, i3, n2, n3);
            ss_local += conc[index];
            r2mean_local += conc[index] * rr;
          }
        }
      }

      // Global values of ss and r2mean must be computed and distributed to all processes
      MPI_Allreduce(&ss_local, &ss, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      MPI_Allreduce(&r2mean_local, &r2mean, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

      ss *= fac;
      r2mean *= fac;

      end = seconds();
    
      if (rank == 0) {
        printf(" %d %17.15f %17.15f Elapsed time per iteration %f \n", istep, r2mean, ss, (end - start) / istep);
      }

      // Output
      plot_data_2d_parallel("concentration", n1, n2, n3, local_n1, local_n1_offset, 2, conc);
      plot_data_1d_parallel("1d_conc", n1, n2, n3, local_n1, local_n1_offset, 3, conc);
    }
  } 
  close_fftw(&fft_h);
  free(diffusivity);
  free(conc);
  free(dconc);
  free(aux1);
  free(aux2);

  MPI_Finalize();
  return 0;
} 

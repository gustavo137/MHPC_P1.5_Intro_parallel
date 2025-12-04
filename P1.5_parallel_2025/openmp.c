#include <stdlib.h>
#include <stdio.h>

#include <omp.h>

#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>


#define N 128


double seconds()
{
  struct timeval tmp;
  double sec;
  gettimeofday( &tmp, (struct timezone *)0 );
  sec = tmp.tv_sec + ((double)tmp.tv_usec)/1000000.0;
  return sec;
}


int main ( int argc, char * argv[] ){

  int th_id, ths_num;
  int i = 0, j = 0, k = 0;
  double t0 = 0.0, t1 = 0.0;

#if DEBUG  
#pragma omp parallel
  {
    th_id = omp_get_thread_num();
    ths_num = omp_get_num_threads();
    
    fprintf( stdout, "\nI am %d over %d threads", th_id, ths_num );
  }
#endif
  
  double * A, * B, *C;

  A = (double *) calloc( N * N, sizeof(double) );
  B = (double *) calloc( N * N, sizeof(double) );
  C = (double *) calloc( N * N, sizeof(double) );

#pragma omp parallel for private(i) 
  for( i = 0; i < N; i++ ){
    A[ i * N + i ] = 1.0;
    B[ i * N + i ] = 1.0;
  }

  double l_sum = 0.0;
  double sum = 0.0;
  
  t0 = seconds();
  
#pragma omp parallel private (l_sum)
  {

#pragma omp for collapse(3)  
    for( i = 0; i < N; i++ ){
      for( j = 0; j < N; j++ ){
	l_sum = 0.0;
	for( k = 0; k < N; k++ ){
	  l_sum += A[ i * N + k ] * B[ k * N + j ];
	}
#pragma omp atomic
	C[ i * N + j ] += l_sum;
      }
    }

#pragma omp single
    fprintf( stdout, "\nTime of esecudion of MM(NxN) = %.3g -- collapse(3) version using atomic", seconds() - t0 );

    t0 = seconds();
    
    for( i = 0; i < N; i++ ){
      for( j = 0; j < N; j++ ){
#pragma omp for reduction (+:sum)
	for( k = 0; k < N; k++ ){
	  sum += A[ i * N + k ] * B[ k * N + j ];
	}
#pragma omp single
	{
	  C[ i * N + j ] += sum;
	  sum = 0.0;
	}
      }
    }
    
#pragma omp single
    fprintf( stdout, "\nTime of esecudion of MM(NxN) = %.3g -- collapse(3) version using reduction", seconds() - t0 );

    
    t0 = seconds();
    
#pragma omp for collapse(2)  
    for( i = 0; i < N; i++ ){
      for( j = 0; j < N; j++ ){
	l_sum = 0.0;
	for( k = 0; k < N; k++ ){
	  // C[ i * N + j ] += A[ i * N + k ] * B[ k * N + j ];
	  l_sum += A[ i * N + k ] * B[ k * N + j ];	  
	}
	C[ i * N + j ] = l_sum;
      }
    }
#pragma omp single
    fprintf( stdout, "\nTime of esecudion of MM(NxN) = %.3g -- collapse(2) version", seconds() - t0 );

    t0 = seconds();    
#pragma omp for 
    for( i = 0; i < N; i++ ){
      for( j = 0; j < N; j++ ){
	l_sum = 0.0;
	for( k = 0; k < N; k++ ){
	  l_sum += A[ i * N + k ] * B[ k * N + j ];	  
	}
	C[ i * N + j ] = l_sum;
      }
    }
#pragma omp single
    fprintf( stdout, "\nTime of esecudion of MM(NxN) = %.3g -- collapse(1) version", seconds() - t0 );
    
    t0 = seconds();
    
#pragma omp for collapse(3)  
    for( i = 0; i < N; i++ ){
      for( j = 0; j < N; j++ ){
	l_sum = 0.0;
	for( k = 0; k < N; k++ ){
	  l_sum += A[ i * N + k ] * B[ k * N + j ];
	}
#pragma omp critical
	C[ i * N + j ] += l_sum;
      }
    }
    
#pragma omp single
    fprintf( stdout, "\nTime of esecudion of MM(NxN) = %.3g -- collapse(3) version using critical", seconds() - t0 );

  }
    
  return 0;
}

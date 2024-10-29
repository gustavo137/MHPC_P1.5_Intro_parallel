#include <stdlib.h>
#include "mpi.h"
#include <string.h>
#include <stdio.h>

#define N 16

void print_matrix( double * mat, int loc_size ){

  int i, j;

  for( i = 0; i < loc_size; i ++){ 
    for( j = 0; j < N; j ++){ 
      
      printf( "%.3g ", mat[ i * N + j ]);
      
    }
    printf( "\n");
  }
}

void print_matrix_par( double * mat, int n_loc, int npes, int me, int rest ){

  int count = 0;

  if( !me ){

    printf( "\n\n");
    print_matrix( mat, n_loc );
    for( count = 1; count < npes; count ++  ){

      if( count == rest ) n_loc -= 1;
      MPI_Recv( mat, n_loc * N, MPI_DOUBLE, count, count, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
      print_matrix( mat, n_loc );
    }
    
  }
  else MPI_Send( mat, n_loc * N, MPI_DOUBLE, 0, me, MPI_COMM_WORLD );
}


int main( int argc, char * argv[] ){

  int i, j; 
  double * mat = NULL;

  // MPI related variables 
  int npes = 1, me = 0;
  int offset = 0, rest = 0;
  int n_loc = N, i_global = 0;

  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &npes );
  MPI_Comm_rank( MPI_COMM_WORLD, &me );

  n_loc = N / npes;
  rest = N % npes;
  if( me < rest ) n_loc += 1;
  else offset = rest;

  mat = (double *) malloc( N * N * sizeof(double) );
  memset( mat, 0, N * N * sizeof(double) );

  //    for( i = 0; i < N; i++ ) -> serial version
  for( i = 0; i < n_loc; i++ ){
    
    i_global = i + ( me * n_loc ) + offset;
    // mat[ i * N + i ] = 1.0; -> serial version
    mat[ i * N + i_global ] = 1.0;
  }

  print_matrix_par( mat, n_loc, npes, me, rest );

  MPI_Finalize();
    
  return 0; 
}

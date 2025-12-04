#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>

#define N 10

void print_mat( int n_loc, int * mat ){

  int i, j;
  for( i = 0; i < n_loc; i++ ){
    for( j = 0; j < N; j++ ){
      
      fprintf( stdout, "%d ", mat[ i * N + j ] );
    }
    fprintf( stdout, "\n");
  }
}

int main( int argc, char * argv[] ){

  int rank, npes;
  MPI_Request request;

  MPI_Init( &argc, &argv );
  MPI_Comm_size( MPI_COMM_WORLD, &npes );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  
  int i, count, n_loc = N / npes, n_loc_recv;
  int offset = 0, rest = N % npes;

  if( rank < rest ) n_loc += 1;
  else offset = rest ;

  int * mat = (int *) calloc( N * n_loc, sizeof(int) );
  int * buf = (int *) calloc( N * n_loc, sizeof(int) );
  
  //initialize local values 
  for( i = 0; i < n_loc; i++ ){

    int i_g = i + ( rank * n_loc ) + offset;
    mat[ i * N + i_g ] = 1;
  } 
    
  if( !rank ){
    
    n_loc_recv = n_loc;

    for( count = 1; count < npes; count++ ){

      if( count == rest ) n_loc_recv -= 1;
      MPI_Irecv( buf, n_loc_recv * N, MPI_INT, count, 100, MPI_COMM_WORLD, &request );
      print_mat( n_loc, mat );
      MPI_Wait( &request, MPI_STATUS_IGNORE );

      //swap pointers
      int * tmp = mat;
      mat = buf;
      buf = tmp;

      if( count == rest ) n_loc = n_loc_recv;

    }
    print_mat( n_loc, mat );

  } MPI_Send( mat, n_loc * N, MPI_INT, 0, 100, MPI_COMM_WORLD);

  MPI_Finalize();

  return 0;
}

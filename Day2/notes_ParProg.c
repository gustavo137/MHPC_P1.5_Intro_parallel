[...]



#define N

doule w=1./N;
double sum = 0.0;

#pragma omp parallel
{
  doule loc_sum = 0.0;
  double A[N]=0.0;

  read(A, file);
  
  init(A); // init vector A  

  for(i->N) abs(A[i]);
  
  #pragma omp for
  for( i = 0; i < N; i++ ){
  
  double x = ( i = 0.5 ) * w;
  dobule f_x = 1./( 1 + x * x );

  loc_sum += f_x;
  
 }

#pragma omp critical
  sum += loc_sum;
}

  
printf( "Pi = %.3", sum * w );

[...]
  

// Implement a print on distributed data

void print( double * mat, int N, int n_loc, int npes){

  double * buffer( N x nloc );
  
  if(!rank){

    print( mat ); // define it later
    for( i = 1; i < npes; i++ ){

      MPI_Recv( buffer, ..., i, ... );
      print( buffer );
    }
  }
  else MPI_Send( mat, ..., 0, ... );
    
  free( buffer );

}


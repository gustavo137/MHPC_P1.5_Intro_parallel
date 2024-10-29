#include <fstream>
#include <iostream>
#include <mpi.h>
#include <vector>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  
  // get the size of the world and rank 
  int NPES;
  MPI_Comm_size(MPI_COMM_WORLD, &NPES);
  int RANK;
  MPI_Comm_rank(MPI_COMM_WORLD, &RANK);
  
  //Create the matrix
  // size of the matrix N*N
  int N{7};
  std::vector<double> Mat(N*N);
  
  int N_LOC = N / NPES;
  int REST = N % NPES;
  //int OFFSET = (RANK < REST) ? RANK : REST;// case a)
  int OFFSET =0;                             // case b)
  if (RANK < REST) {
    N_LOC += 1;
    OFFSET=RANK; // case b)
  }else {
   OFFSET=REST;
  }

  //int i_GLOBAL = RANK * (N / NPES) + OFFSET; // case a)
  int i_GLOBAL = (N_LOC*RANK) + OFFSET;// case b)

  // Recorrer filas locales
  for (int i_local = 0; i_local < N_LOC; ++i_local) {
    int i_global_actual = i_GLOBAL + i_local;
    for (int j = 0; j < N; ++j) {
      if (i_global_actual == j) {
        Mat[i_global_actual * N + j] = 1.0;
      } else {
        Mat[i_global_actual * N + j] = 0.0;
      }
    }
  }
 
  //dummy print
  if(RANK==1){
    for(int i=0;i<N_LOC;++i){
      for(int j=0;j<N;j++){
        std::cout<<Mat[i*N+j]<<" ";
      }
      std::cout<<std::endl;
    }
  }

 MPI_Finalize(); 
return 0;
}

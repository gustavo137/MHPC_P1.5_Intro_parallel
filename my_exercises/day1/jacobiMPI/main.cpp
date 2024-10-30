#include "tools.hpp"
#include <mpi.h>

int main(int argc, char **argv){
  MPI_Init(&argc, &argv);
  int rank, world_size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Things to the matriz
  std::vector<double> conini = {0, 100};
  size_t dim{100}; // 100// for leonardo set 10000
  size_t ite{1000};
  size_t printInterval{200};

  // Set the things to each rank
  size_t dim_local = dim / world_size;
  size_t rest = dim % world_size;
  size_t start_row;

  if (rank < rest) {
    dim_local += 1;
    start_row = rank * dim_local;
  } else {
    start_row = rank * dim_local + rest;
  }

  // Create the mesh class element
  CMesh<double> Matrix(dim, dim_local, start_row, conini, rank, world_size);
  //Matrix.print_in_serial();
  //Matrix.print_in_parallel();
  //
  {CParallel_Timer t("Solving_jacobiMPI");
  CSolver<double> solver(Matrix);
  solver.jacobi(Matrix, ite, printInterval);
  }
  //
  if(rank==0){
  //std::cout<<"finished"<<std::endl;
  }
  // print the times
  CParallel_Timer::gather_and_process_times(rank, world_size);

  MPI_Finalize();
  return 0;
}

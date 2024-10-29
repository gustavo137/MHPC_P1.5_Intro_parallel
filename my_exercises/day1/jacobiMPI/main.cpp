#include "toolsMPI.hpp"
#include "Timer.hpp"
#include <mpi.h>

//to compile: mpic++ -fopenmp main.cpp -o main.x
 
// to run:mpirun -np <num_procesos> ./main.x


int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  std::vector<double> conini = {0, 100};
  size_t dim = 1000;
  size_t ite = 100;
  size_t printInterval = 200;

  CMesh<double> Matrix(dim, conini);

  {
    CSimple_timer t("Jacobi using MPI and OpenMP");
    CSolver<double> solver(Matrix);
    solver.jacobi(Matrix, ite, printInterval);
  }

  CSimple_timer::print_timing_results();
  MPI_Finalize();
  return 0;
}


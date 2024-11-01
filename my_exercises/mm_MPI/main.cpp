#include "tools_mm.hpp"
#include "mpi_tools.hpp"
#include "Parallel_Timer.hpp"

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int N{8};
  // to have different matrices
  std::srand(time(0));
  CMatrix<double> A(N), B(N);
  A.fill();
  B.fill();  
  //std::cout << "A is" << std::endl;
  //std::cout << A << std::endl;
  //std::cout << "B is" << std::endl;
  //std::cout << B << std::endl;
 
  {Parallel_Timer t("Matrix mult.");
  auto C = A*B;
  }
  Parallel_Timer::gather_timing_data(MPI_COMM_WORLD, 0);
  //std::cout<<A*B<<std::endl;

  MPI_Finalize();
  return 0;
}

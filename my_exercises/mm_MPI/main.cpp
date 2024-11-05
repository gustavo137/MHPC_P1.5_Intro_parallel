#include "tools_mm.hpp"

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank, npes;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &npes);

  int N=2; // size of the matriz by default
  // to pass a new value for example ./main.x 10
  if (argc > 1) {
    N = std::stoi(argv[1]);
  }
  // at this point we will use onli npes as multiplo de N, with square matrices
  // N*N, to have remainder=0;
  // npes as multiplo of N, example if N=10, then npes= 1,2,5,10.
  int remainder = N % npes;
  std::vector<int> N_locs(npes);
  std::vector<int> row_offsets(npes + 1, 0);
  // fill this vectors
  for (int i = 0; i < npes; i++) {
    N_locs[i] = (i < remainder) ? (N / npes + 1) : (N / npes);
    row_offsets[i + 1] = row_offsets[i] + N_locs[i];
  }

  // variables to pass to the multiplication in parallel
  int N_loc = N_locs[rank];
  int start_row = row_offsets[rank];

  // random  seed to have different matrices
  // std::srand(time(0));
  //  Create the matrices A, B  and C
  CMatrix<double> A(N_loc, N), B(N_loc, N), C(N_loc, N), D(N_loc, N);
  // fill A and B with numbers from 0 to 10
  A.fill(10.0);
  B.fill(10.0);

  print_in_parallel(A,rank,npes,"A");
  print_in_parallel(B,rank,npes,"B");

  //

  {
    Parallel_Timer t("mm naive.");
    multiply_parallel_naive(A, B, C, N, N_loc, npes, row_offsets, N_locs);
  }

  {
    Parallel_Timer t("mm blas");
    multiply_parallel_blas(A, B, D, N, N_loc, npes, row_offsets, N_locs);
  }
  print_in_parallel(C,rank,npes,"C naiv");
  print_in_parallel(D,rank,npes,"C blas");

  MPI_Barrier(MPI_COMM_WORLD);
  Parallel_Timer::gather_timing_data(MPI_COMM_WORLD, 0);
  // std::cout<<A*B<<std::endl;

  MPI_Finalize();
  return 0;
}


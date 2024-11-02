#include "tools_mm.hpp"
#include "mpi_tools.hpp"
#include "Parallel_Timer.hpp"

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank, npes;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&npes); 
  
  int N{2}; //size of the matriz by default 
  // to pass a new value for example ./main.x 10 
  if(argc>1){
    N=std::atoi(argv[1]);
  }
  // at this point we will use onli npes as multiplo de N, with square matrices N*N, to have remainder=0;
  //npes as multiplo of N, example if N=10, then npes= 1,2,5,10.
  int remainder = N % npes;
  std::vector<int> N_locs(npes);
  std::vector<int> row_displacements(npes + 1, 0); 
  //fill this vectors 
  for(int i=0;i<npes;i++){
    N_locs[i]=(i<remainder)? (N/npes + 1):(N/npes);
    row_displacements[i+1]=row_displacements[i]+N_locs[i];
  }  
  
  //variables to pass to the multiplication in parallel
  int N_loc = N_locs[rank];
  int start_row = row_displacements[rank];
   
  //random  seed to have different matrices
  //std::srand(time(0));
  // Create the matrices A, B  and C
  CMatrix<int> A(N_loc,N), B(N_loc,N), C(N_loc,N);
  // fill A and B with numbers from 0 to 10
  A.fill(10);
  B.fill(10);
  //std::cout<<A<<std::endl;
  
  //this is the main function
  multiply_in_parallel(A,B,C,N_loc,N,npes,row_displacements,N_locs);
  
  
  //std::cout << "A is" << std::endl;
  print_in_parallel(A,rank,npes);
  //std::cout << A << std::endl;
  //std::cout << "B is" << std::endl;
  print_in_parallel(B,rank,npes);
  //std::cout << B << std::endl;
  //std::cout << "C is:"<<std::endl;
  //std::cout << C << std::endl;
  //std::cout << "C parallel:"<<std::endl;
  print_in_parallel(C,rank,npes);
  /* 
  {Parallel_Timer t("Matrix mult.");
  auto C = A*B;
  }

  //MPI_Barrier(MPI_COMM_WORLD);
  CMatrix<double> A2(2*N), B2(N);
  A2.fill(0,2*N);
  B2.fill(0,2*N);
  
  {Parallel_Timer t("Matrix mult. 2");
  auto C2 = A2*B2;
  }  
  Parallel_Timer::gather_timing_data(MPI_COMM_WORLD, 0);
  //std::cout<<A*B<<std::endl;
  */
  MPI_Finalize();
  return 0;
}

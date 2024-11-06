#include "tools.hpp"

//to see info:
/*
https://users.cs.utah.edu/~hari/teaching/paralg/tutorial/05_Cannons.html
*/

int main(int argc, char **argv) {
  int rank, size, q;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Size of the matrix (N x N)
  int N = 16; // size by default 
  if(argc>1){
     N = std::atoi(argv[1]);
  }
  //remenber that npes = q x q, for example, 1, 4, 16 with N a multiple of this 
  // q=sqrt(npes)
  q = ((int)(std::sqrt(size)));
  if (q * q != size) {
    if (rank == 0) {
      std::cerr<< "------------------------------------"<<std::endl;
      std::cerr << "npes needs to be a perfect square, 1, 4, 16, and 64." << std::endl;
      std::cerr<< "------------------------------------"<<std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int N_loc = N / q;// size of each block can be called N_loc 
  int dims[] = {q, q}; // dimention of the grid, has the total of process in each dimention
  int periods[] = {true, true};//0 ciclic conectivity, 1 periodicity 
  MPI_Comm cart_comm;// communicator 
  // reorder 1, to reorder the clasification and give us better better performances
  // MPI_Cart_create(MPI_Comm comm, int ndims, int dims[], int periods[], int reorder, MPI_Comm &cart_comm);
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);
  // now traslate rank to coordinates. 

  int this_rank;
  MPI_Comm_rank(cart_comm, &this_rank);
   
  int coords[2];
  MPI_Cart_coords(cart_comm, this_rank, 2, coords);
  
  // shifts to left, rignt, up and down
  int left, right, up, down;
  // MPI_Cart_shift(MPI_Comm, int direction, displacement, &rank_source ,&rank_dest)
  // direction the most importan part: to shift rows -> 1, columm ->0
  // the amoung of shift 1 or -1. 
  MPI_Cart_shift(cart_comm, 1, coords[1], &left, &right);
  MPI_Cart_shift(cart_comm, 0, coords[0], &up, &down);
  // this function returns rank_source and rank_dest, for example 
  //if we have p1,p2,p3,p4,p5,p6, si rank actual is 3, and desplacements is +-2
  // then rank_source = p1 and rank_dest =p5

  //CMatrix A,B,C,D remember that blas only work with doubles 
  CMatrix<double> A(N_loc);
  CMatrix<double> B(N_loc);
  CMatrix<double> C(N_loc);
  CMatrix<double> D(N_loc);
  // fill the matrices A and B with random data from 0 to 10.0
  A.fill(10.0);
  B.fill(10.0);

  // Print the matrices A and B to see and check 
  // gather_and_print_matrix(A, N_loc, N, rank, q, MPI_COMM_WORLD, "A");
  // gather_and_print_matrix(B, N_loc, N, rank, q, MPI_COMM_WORLD, "B");

  // in this past only use Isen and I recv 
  {Parallel_Timer t("total time cannon blas");
   multiply_cannon(A,B,C,N,cart_comm);
  } 
  
  // Print the matrices A and B again to see if it has the original position
  //gather_and_print_matrix(A, N_loc, N, rank, q, MPI_COMM_WORLD, "A");
  //gather_and_print_matrix(B, N_loc, N, rank, q, MPI_COMM_WORLD, "B");
   
  
  // Print the C matrix with the multiplication 
  //gather_and_print_matrix(C, N_loc, N, rank, q, MPI_COMM_WORLD, "C");
  //gather_and_print_matrix(D, N_loc, N, rank, q, MPI_COMM_WORLD, "D");

  // Print times
  if(rank==0){
    std::cout<<"Comm time = total time - comp time."<< std::endl;
  }
  Parallel_Timer::gather_timing_data(MPI_COMM_WORLD, 0);

  MPI_Finalize();
  return 0;
}

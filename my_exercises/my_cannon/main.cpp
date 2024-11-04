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
  q = ((int)(std::sqrt(size)));
  if (q * q != size) {
    if (rank == 0) {
      std::cerr << "npes needs to be a perfect square." << std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int BLOCK_SIZE = N / q;// size of each block 
  int dims[2] = {q, q}; // dimention of the grid, has the total of process in each dimention
  int periods[2] = {1, 1};//0 ciclic conectivity, 1 periodicity 
  MPI_Comm cart_comm;// communicator 
  // reorder 1, to reorder the clasification and give us better better performances
  // MPI_Cart_create(MPI_Comm comm, int ndims, int dims[], int periods[], int reorder, MPI_Comm &cart_comm);
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);
  // now traslate rank to coordinates. 
  int coords[2];
  MPI_Cart_coords(cart_comm, rank, 2, coords);
  
  // shifts to left, rignt, up and down
  int left, right, up, down;
  // MPI_Cart_shift(MPI_Comm, int direction, &rank_source ,&rank_dest)
  // direction the most importan part: to shift rows -> 0, columm ->1
  // the amoung of shift 1 or -1. 
  MPI_Cart_shift(cart_comm, 1, 1, &left, &right);
  MPI_Cart_shift(cart_comm, 0, 1, &up, &down);
  // this function returns rank_source and rank_dest, for example 
  //if we have p1,p2,p3,p4,p5,p6, si rank actual is 3, and desplacements is +-2
  // then rank_source = p1 and rank_dest =p5

  //CMatrix A,B,C,D remember that blas only work with doubles 
  CMatrix<double> A(BLOCK_SIZE);
  CMatrix<double> B(BLOCK_SIZE);
  CMatrix<double> C(BLOCK_SIZE);
  CMatrix<double> D(BLOCK_SIZE);
  // fill the matrices A and B with random data from 0 to 10.0
  A.fill(10.0);
  B.fill(10.0);

  // Print the matrices A and B to see and check 
  // gather_and_print_matrix(A, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "A");
  // gather_and_print_matrix(B, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "B");

  {Parallel_Timer t("Tolat time: ");
    //Initial displacement of blocks A and B
    {Parallel_Timer t("Comm time: "); // Comm time = comm time - comp time.
      for (int i = 0; i < coords[0]; ++i) {
        MPI_Sendrecv_replace(A.data(), BLOCK_SIZE * BLOCK_SIZE,
                             mpi_type<double>(), left, 0, right, 0, cart_comm,
                             MPI_STATUS_IGNORE);
      }
      for (int i = 0; i < coords[1]; ++i) {
        MPI_Sendrecv_replace(B.data(), BLOCK_SIZE * BLOCK_SIZE,
                             mpi_type<double>(), up, 0, down, 0, cart_comm,
                             MPI_STATUS_IGNORE);
      }
       
      // Make the matrix multiplication of each block using naive or blas 
      for (int step = 0; step < q; ++step) {
        {Parallel_Timer t("Comp time: ");
          multiply_blocks_blas(A, B, C, BLOCK_SIZE);
          //multiply_blocks(A, B, D, BLOCK_SIZE);
        }
        // return the block of A and B to the original position 
        // this is the last exchange 
        MPI_Sendrecv_replace(A.data(), BLOCK_SIZE * BLOCK_SIZE,
                             mpi_type<double>(), left, 0, right, 0, cart_comm,
                             MPI_STATUS_IGNORE);
        MPI_Sendrecv_replace(B.data(), BLOCK_SIZE * BLOCK_SIZE,
                             mpi_type<double>(), up, 0, down, 0, cart_comm,
                             MPI_STATUS_IGNORE);
      }
    }
  }
  // Print the matrices A and B again to see if it has the original position
  //gather_and_print_matrix(A, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "A");
  //gather_and_print_matrix(B, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "B");
   
  
  // Print the C matrix with the multiplication 
  //gather_and_print_matrix(C, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "C");
  //gather_and_print_matrix(D, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "D");

  // Print times
  Parallel_Timer::gather_timing_data(MPI_COMM_WORLD, 0);

  MPI_Finalize();
  return 0;
}

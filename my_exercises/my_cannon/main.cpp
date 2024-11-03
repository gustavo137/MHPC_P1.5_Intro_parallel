#include "tools.hpp"

int main(int argc, char **argv) {
  int rank, size, q;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Definir el tamaño de la matriz (N x N)
  int N = 16; // size by default 
  if(argc>1){
     N = std::atoi(argv[1]);
  }
  q = static_cast<int>(std::sqrt(size));
  if (q * q != size) {
    if (rank == 0) {
      std::cerr << "npes needs to be a perfect square." << std::endl;
    }
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }

  int BLOCK_SIZE = N / q;
  int dims[2] = {q, q};
  int periods[2] = {1, 1};
  MPI_Comm cart_comm;
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);

  int coords[2];
  MPI_Cart_coords(cart_comm, rank, 2, coords);

  int left, right, up, down;
  MPI_Cart_shift(cart_comm, 1, 1, &left, &right);
  MPI_Cart_shift(cart_comm, 0, 1, &up, &down);

  // Crear matrices A, B, y C como objetos de CMatrix con tipo `double`
  CMatrix<double> A(BLOCK_SIZE);
  CMatrix<double> B(BLOCK_SIZE);
  CMatrix<double> C(BLOCK_SIZE);
  CMatrix<double> D(BLOCK_SIZE);
  // Llenar A y B con valores aleatorios en el rango de 0 a 10
  A.fill(10.0);
  B.fill(10.0);

  // Imprimir las matrices A y B antes de la multiplicación
  // gather_and_print_matrix(A, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "A");
  // gather_and_print_matrix(B, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "B");

  {Parallel_Timer t("Tolat time: ");
    // Desplazamiento inicial de los bloques de A y B
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

      // Realizar la multiplicación de bloques en el algoritmo de Cannon
      for (int step = 0; step < q; ++step) {
        {Parallel_Timer t("Comp time: ");
          multiply_blocks_blas(A, B, C, BLOCK_SIZE);
          //multiply_blocks(A, B, D, BLOCK_SIZE);
        }
        MPI_Sendrecv_replace(A.data(), BLOCK_SIZE * BLOCK_SIZE,
                             mpi_type<double>(), left, 0, right, 0, cart_comm,
                             MPI_STATUS_IGNORE);
        MPI_Sendrecv_replace(B.data(), BLOCK_SIZE * BLOCK_SIZE,
                             mpi_type<double>(), up, 0, down, 0, cart_comm,
                             MPI_STATUS_IGNORE);
      }
    }
  }
  // Imprimir la matriz C después de la multiplicación
  //gather_and_print_matrix(C, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "C");
  //gather_and_print_matrix(D, BLOCK_SIZE, N, rank, q, MPI_COMM_WORLD, "D");

  // Print times
  Parallel_Timer::gather_timing_data(MPI_COMM_WORLD, 0);

  MPI_Finalize();
  return 0;
}

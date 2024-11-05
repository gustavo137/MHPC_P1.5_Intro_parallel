#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "Parallel_Timer.hpp"
#include "mpi_tools.hpp"
#include <algorithm>
#include <cblas.h> // use -lblas to compile time
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <omp.h>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

// Función auxiliar para obtener el tipo de datos de MPI correspondiente al tipo
// T
template <typename T> MPI_Datatype mpi_type();

template <> MPI_Datatype mpi_type<int>() { return MPI_INT; }

template <> MPI_Datatype mpi_type<double>() { return MPI_DOUBLE; }

template <> MPI_Datatype mpi_type<float>() { return MPI_FLOAT; }

// Clase CMatrix para representar una matriz de tamaño N x N y llenarla con
// valores aleatorios
template <typename T> class CMatrix {
public:
  std::vector<T> matrix;
  size_t N;
  CMatrix(size_t N0) : N(N0), matrix(N0 * N0, T{}) {}
  void fill(T max0_to);
  T *data() {
    return matrix.data();
  } // to aboid this only use A.matrix.data() in each function
};

template <typename T> void CMatrix<T>::fill(T max0_to) {
  if constexpr (std::is_same_v<int, T>) {
    std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, max0_to);
    std::generate(matrix.begin(), matrix.end(), [&]() { return dist(engine); });
  } else if constexpr (std::is_same_v<double, T>) {
    std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(1.0, max0_to);
    std::generate(matrix.begin(), matrix.end(), [&]() { return dist(engine); });
  } else if constexpr (std::is_same_v<float, T>) {
    std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<float> dist(1.0f, max0_to);
    std::generate(matrix.begin(), matrix.end(), [&]() { return dist(engine); });
  } else {
    std::cerr << "Type not supported: only int, double, and float."
              << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

template <typename T>
void multiply_blocks(const CMatrix<T> &A, const CMatrix<T> &B, CMatrix<T> &C,
                     int BLOCK_SIZE) {
#pragma omp parallel for
  for (int i = 0; i < BLOCK_SIZE; ++i) {
    for (int j = 0; j < BLOCK_SIZE; ++j) {
      for (int k = 0; k < BLOCK_SIZE; ++k) {
        C.matrix[i * BLOCK_SIZE + j] +=
            A.matrix[i * BLOCK_SIZE + k] * B.matrix[k * BLOCK_SIZE + j];
      }
    }
  }
}
template <typename T>
void multiply_blocks_blas(const CMatrix<T> &A, const CMatrix<T> &B,
                          CMatrix<T> &C, int BLOCK_SIZE) {
  static_assert(std::is_same<T, double>::value,
                "cblas_dgemm requires double data type");

  cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, BLOCK_SIZE, BLOCK_SIZE,
              BLOCK_SIZE, 1.0, A.matrix.data(), BLOCK_SIZE, //
              B.matrix.data(), BLOCK_SIZE,                  //
              1.0,                          // because we want to add to C
              C.matrix.data(), BLOCK_SIZE); //
}
template <typename T>
void gather_and_print_matrix(const CMatrix<T> &local_matrix, int BLOCK_SIZE,
                             int N, int rank, int q, MPI_Comm comm,
                             const std::string &matrix_name) {
  int size = BLOCK_SIZE * BLOCK_SIZE;
  std::vector<T> full_matrix;

  if (rank == 0) {
    full_matrix.resize(N * N);
  }

  std::vector<int> displs(q * q), recv_counts(q * q, size);
  if (rank == 0) {
    for (int i = 0; i < q; ++i) {
      for (int j = 0; j < q; ++j) {
        displs[i * q + j] = i * N * BLOCK_SIZE + j * BLOCK_SIZE;
      }
    }
  }

  MPI_Gatherv(local_matrix.matrix.data(), size, mpi_type<T>(),
              full_matrix.data(), recv_counts.data(), displs.data(),
              mpi_type<T>(), 0, comm);

  if (rank == 0) {
    std::cout << "Matrix " << matrix_name << ":" << std::endl;
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        std::cout << full_matrix[i * N + j] << " ";
      }
      std::cout << std::endl;
    }
  }
}

void multiply_cannon(CMatrix<double> &A, CMatrix<double> &B, CMatrix<double> &C,
                     int N, MPI_Comm comm_cart) {
  int myrank;
  MPI_Comm_rank(comm_cart, &myrank);

  int dims[2], periods[2], coords[2];
  MPI_Cart_get(comm_cart, 2, dims, periods, coords);

  int q = dims[0]; // Number of processes in one dimension

  int N_loc = N / q; // Local matrix size

  // Get ranks of neighbors for shifts
  int left, right, up, down;
  MPI_Cart_shift(comm_cart, 1, coords[0], &left,
                 &right); // Shift along columns (left/right)
  MPI_Cart_shift(comm_cart, 0, coords[1], &up,
                 &down); // Shift along rows (up/down)

  // Prepare the buffers for communication
  std::vector<double> send_buf_A(N_loc * N_loc);
  std::vector<double> send_buf_B(N_loc * N_loc);
  std::vector<double> recv_buf_A(N_loc * N_loc);
  std::vector<double> recv_buf_B(N_loc * N_loc);

  // Initial alignment
  // Shift A left by coords[0] steps
  MPI_Sendrecv_replace(A.matrix.data(), N_loc * N_loc, MPI_DOUBLE, left, 0,
                       right, 0, comm_cart, MPI_STATUS_IGNORE);

  // Shift B up by coords[1] steps
  MPI_Sendrecv_replace(B.matrix.data(), N_loc * N_loc, MPI_DOUBLE, up, 0, down,
                       0, comm_cart, MPI_STATUS_IGNORE);

  // Copy the initial A and B blocks into send buffers
  std::copy(A.matrix.begin(), A.matrix.end(), send_buf_A.begin());
  std::copy(B.matrix.begin(), B.matrix.end(), send_buf_B.begin());

  MPI_Cart_shift(comm_cart, 1, 1, &left,
                 &right); // Shift along columns (left/right)
  MPI_Cart_shift(comm_cart, 0, 1, &up, &down); // Shift along rows (up/down)

  for (int step = 0; step < q; step++) {
    // Start non-blocking sends and receives
    MPI_Request send_req_A, recv_req_A, send_req_B, recv_req_B;

    // Send A left, receive from right
    MPI_Isend(send_buf_A.data(), N_loc * N_loc, MPI_DOUBLE, left, 0, comm_cart,
              &send_req_A);
    MPI_Irecv(recv_buf_A.data(), N_loc * N_loc, MPI_DOUBLE, right, 0, comm_cart,
              &recv_req_A);

    // Send B up, receive from down
    MPI_Isend(send_buf_B.data(), N_loc * N_loc, MPI_DOUBLE, up, 0, comm_cart,
              &send_req_B);
    MPI_Irecv(recv_buf_B.data(), N_loc * N_loc, MPI_DOUBLE, down, 0, comm_cart,
              &recv_req_B);

    {
      Parallel_Timer t("comp time cannon");
      // Compute C += A * B
      cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, N_loc, N_loc,
                  N_loc, 1.0, sendbufA.data(), N_loc, sendbufB.data(), N_loc,
                  1.0, C.matrix.data(), N_loc);
    }

    // Wait for communication to complete
    MPI_Wait(&send_req_A, MPI_STATUS_IGNORE);
    MPI_Wait(&recv_req_A, MPI_STATUS_IGNORE);
    MPI_Wait(&send_req_B, MPI_STATUS_IGNORE);
    MPI_Wait(&recv_req_B, MPI_STATUS_IGNORE);

    // Swap received buffers with send buffers for next iteration
    std::swap(send_buf_A, recv_buf_A);
    std::swap(send_buf_B, recv_buf_B);
  }
}
#endif

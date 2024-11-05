#include "mpi_tools.hpp"
#include <algorithm>
#include <cblas.h> // to compile remember to use -lopenblas or -lblas
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <omp.h>
#include <random>
#include <type_traits>
#include <vector>

#include "Parallel_Timer.hpp"

template <typename T> class CMatrix {
public:
  std::vector<T> matrix;
  int height, width;
  CMatrix(int height0, int width0); //: size(N),data(N*N){};
  //~CMatrix();
  void fill(T max0_to); // function fill with diferents types from 0 to max0_to
  // new operators
  // check if Can I print using this function. Other with create a function
  // print_in_parallel();
  template <typename U>
  friend std::ostream &operator<<(std::ostream &os, const CMatrix<U> &p);
  void resize(int new_height, int new_width,
              T fill_with = 0); // this function change the value of height
                                // and width, new size con value "fill_with"
};

////////
// Constructor
template <typename T>
CMatrix<T>::CMatrix(int height0, int width0)
    : height(height0), width(width0), matrix(height0 * width0, 0) {}
///////
///////// fill(start, end)
template <typename T> void CMatrix<T>::fill(T max0_to) {
  if constexpr (std::is_same_v<int, T>) {
    std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, max0_to);
    auto generate_random_number = [&]() { return dist(engine); };
    // Llenar toda la matriz
    std::generate(matrix.begin(), matrix.end(), generate_random_number);
  } else if constexpr (std::is_same_v<double, T>) {
    std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<double> dist(1.0, max0_to);
    auto generate_random_number = [&]() { return dist(engine); };
    std::generate(matrix.begin(), matrix.end(), generate_random_number);
  } else if constexpr (std::is_same_v<float, T>) {
    std::mt19937 engine(std::random_device{}());
    std::uniform_real_distribution<float> dist(1.0f, max0_to);
    auto generate_random_number = [&]() { return dist(engine); };
    std::generate(matrix.begin(), matrix.end(), generate_random_number);
  } else {
    std::cerr << "Type no soported: only int, double and float:" << std::endl;
    exit("FAILURE");
  }
}
///////// end fill()

/////// Sobrecarga del operador << para imprimir la matriz
template <typename U>
std::ostream &operator<<(std::ostream &os, const CMatrix<U> &p) {
  for (size_t i = 0; i < p.height; i++) {
    for (size_t j = 0; j < p.width; j++) {
      os << p.mat[p.height * i + j] << " ";
    }
    os << std::endl;
  }
  // os<<p.mat[0]<<" "<<p.mat[1]<<" "<<p.mat[2]<<" "<<p.mat[3]<<std::endl;
  return os;
}
////////////// en <<
///////////////////// matresize() /////
template <typename T>
void CMatrix<T>::resize(int new_height, int new_width, T fill_with) {
  height = new_height;
  width = new_width;
  matrix.resize(new_height * new_width, fill_with);
}
//////////// end matresize()////////
int global_row(int local_row, int rank, int world_size, int N) {
  int rows_per_process = N / world_size;
  int extra_rows = N % world_size;
  int offset = rank * rows_per_process + std::min(rank, extra_rows);
  return local_row + offset;
}
////// print in parallel //////////////////
template <typename T>
void print_in_parallel(const CMatrix<T> &local_matrix, int rank,
                       int world_size, const std::string& matrix_name) {
  int width = local_matrix.width;
  if (width < 16) {
    int local_height = local_matrix.height;
    MPI_Datatype mpi_type = get_mpi_datatype<T>();

    if (rank == 0) {
      std::cout << "Matrix " << matrix_name << ":" << std::endl;
      // Root process prints its own matrix
      for (int local_row = 0; local_row < local_height; ++local_row) {
        // Print the row
        for (int col = 0; col < width; ++col) {
          std::cout << local_matrix.matrix[local_row * width + col] << " ";
        }
        std::cout << std::endl;
      }

      // Now receive matrices from other processes
      for (int source_rank = 1; source_rank < world_size; ++source_rank) {
        // Receive the number of rows from the source rank
        int source_local_height;
        MPI_Recv(&source_local_height, 1, MPI_INT, source_rank, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive and print each row
        for (int i = 0; i < source_local_height; ++i) {
          std::vector<T> row_data(width);
          MPI_Recv(row_data.data(), width, mpi_type, source_rank, 0,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          // Print the received row
          for (int col = 0; col < width; ++col) {
            std::cout << row_data[col] << " ";
          }
          std::cout << std::endl;
        }
      }
    } else {
      // Other processes send their local matrices to the root
      // Send the number of rows to the root
      MPI_Send(&local_height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

      // Send each row to the root
      for (int local_row = 0; local_row < local_height; ++local_row) {
        MPI_Send(&local_matrix.matrix[local_row * width], width, mpi_type, 0, 0,
                 MPI_COMM_WORLD);
      }
    }
  } else {
    // For width >= 16, write to binary file
    MPI_Datatype mpi_type = get_mpi_datatype<T>();
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, "matrix.bin",
                  MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

    // Calculate offset in the file
    int rows_per_process = width / world_size;
    int extra_rows = width % world_size;
    int offset_rows = 0;

    for (int i = 0; i < rank; i++) {
      offset_rows += rows_per_process + (i < extra_rows ? 1 : 0);
    }

    MPI_Offset file_offset =
        static_cast<MPI_Offset>(offset_rows * width * sizeof(T));

    MPI_File_write_at_all(fh, file_offset, local_matrix.matrix.data(),
                          local_matrix.height * width, mpi_type,
                          MPI_STATUS_IGNORE);

    MPI_File_close(&fh);
  }
}

////// MM naive /////
template <typename T>
void multiply_parallel_naive(CMatrix<T> &A, CMatrix<T> &B, CMatrix<T> &C, int N,
                             int N_loc, int Np,
                             const std::vector<int> &row_offsets,
                             const std::vector<int> &N_locs) {
  // Create the vertical stripe of B (size N x N_loc)
  CMatrix<T> B_stripe(N, N_loc);

  // Loop over each vertical stripe of B
  for (int stripe = 0; stripe < Np; ++stripe) {
    int col_start =
        row_offsets[stripe]; // Starting column index for process stripe
    int col_count =
        N_locs[stripe]; // Number of columns assigned to process stripe

    {
      Parallel_Timer t("Comm mm naive");
      // Resize the vertical stripe of B (size N x col_count)
      B_stripe.resize(N, col_count);

      // Each process extracts the columns [col_start, col_start + col_count)
      // from its B_local Pack data into send buffer
      std::vector<T> sendbuf(N_loc * col_count);
      for (int i = 0; i < N_loc; ++i) {
        for (int j = 0; j < col_count; ++j) {
          sendbuf[i * col_count + j] = B.matrix[i * N + col_start + j];
        }
      }

      // Prepare counts and displacements for MPI_Allgatherv
      std::vector<int> recvcounts(Np);
      std::vector<int> displs(Np);
      int total_recvcount = 0;
      for (int p = 0; p < Np; ++p) {
        recvcounts[p] =
            N_locs[p] *
            col_count; // Each process sends N_locs[p] x col_count elements
        displs[p] = total_recvcount;
        total_recvcount += recvcounts[p];
      }

      // Receive buffer to collect vertical stripe of B
      std::vector<T> recvbuf(total_recvcount);

      // Allgather the necessary columns to reconstruct the vertical stripe of B
      mpi_allgatherv<T>(sendbuf, N_loc * col_count, recvbuf, recvcounts, displs,
                        MPI_COMM_WORLD);

      // Reconstruct the vertical stripe of B (size N x col_count)
      int offset = 0;
      for (int p = 0; p < Np; ++p) {
        int rows_p = N_locs[p];
        for (int i = 0; i < rows_p; ++i) {
          int global_row = row_offsets[p] + i;
          for (int j = 0; j < col_count; ++j) {
            B_stripe.matrix[global_row * col_count + j] =
                recvbuf[offset + i * col_count + j];
          }
        }
        offset += rows_p * col_count;
      }
    }
    {
      Parallel_Timer t("Comp mm naive");
      // Multiply A_local (N_loc x N) with B_stripe (N x col_count) and store
      // directly into C
      for (int i = 0; i < N_loc; ++i) {
        for (int j = 0; j < col_count; ++j) {
          T sum = 0;
          for (int l = 0; l < N; ++l) {
            sum += A.matrix[i * N + l] * B_stripe.matrix[l * col_count + j];
          }
          C.matrix[i * N + col_start + j] = sum;
        }
      }
    }
  }
}

template <typename T>
void multiply_parallel_blas(CMatrix<T> &A, CMatrix<T> &B, CMatrix<T> &C, int N,
                            int N_loc, int Np,
                            const std::vector<int> &row_offsets,
                            const std::vector<int> &N_locs) {
  // Create the vertical stripe of B (size N x N_loc)
  CMatrix<T> B_stripe(N, N_loc);

  // Loop over each vertical stripe of B
  for (int stripe = 0; stripe < Np; ++stripe) {
    int col_start =
        row_offsets[stripe]; // Starting column index for process stripe
    int col_count =
        N_locs[stripe]; // Number of columns assigned to process stripe

    {
      Parallel_Timer t("comm mm blas");
      // Resize the vertical stripe of B (size N x col_count)
      B_stripe.resize(N, col_count);

      // Each process extracts the columns [col_start, col_start + col_count)
      // from its B_local Pack data into send buffer
      std::vector<T> sendbuf(N_loc * col_count);
      for (int i = 0; i < N_loc; ++i) {
        for (int j = 0; j < col_count; ++j) {
          sendbuf[i * col_count + j] = B.matrix[i * N + col_start + j];
        }
      }

      // Prepare counts and displacements for MPI_Allgatherv
      std::vector<int> recvcounts(Np);
      std::vector<int> displs(Np);
      int total_recvcount = 0;
      for (int p = 0; p < Np; ++p) {
        recvcounts[p] =
            N_locs[p] *
            col_count; // Each process sends N_locs[p] x col_count elements
        displs[p] = total_recvcount;
        total_recvcount += recvcounts[p];
      }

      // Receive buffer to collect vertical stripe of B
      std::vector<T> recvbuf(total_recvcount);

      // Allgather the necessary columns to reconstruct the vertical stripe of B
      mpi_allgatherv<T>(sendbuf, N_loc * col_count, recvbuf, recvcounts, displs,
                        MPI_COMM_WORLD);

      // Reconstruct the vertical stripe of B (size N x col_count)
      int offset = 0;
      for (int p = 0; p < Np; ++p) {
        int rows_p = N_locs[p];
        for (int i = 0; i < rows_p; ++i) {
          int global_row = row_offsets[p] + i;
          for (int j = 0; j < col_count; ++j) {
            B_stripe.matrix[global_row * col_count + j] =
                recvbuf[offset + i * col_count + j];
          }
        }
        offset += rows_p * col_count;
      }
    }
    {
      Parallel_Timer t("comp mm blas");
      // Multiply A_local (N_loc x N) with B_stripe (N x col_count) and store
      // directly into C
      cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, N_loc, col_count,
                  N, 1.0, A.matrix.data(), N, B_stripe.matrix.data(), col_count,
                  0.0, &C.matrix[col_start], N);
    }
  }
}

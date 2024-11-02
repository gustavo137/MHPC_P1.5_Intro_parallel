#ifndef MATRIX_OVERLOADING_H
#define MATRIX_OVERLOADING_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <type_traits>
#include <cblas.h>
#include "mpi_utils.hpp"
#include "simple_timer.hpp"

const int DEFAULT_SIZE = 4;

template <typename T>
class Matrix {
public:
    int width, height;
    std::vector<T> matrix;

    Matrix() : Matrix(DEFAULT_SIZE, DEFAULT_SIZE) {}
    Matrix(int height0, int width0) : height(height0), width(width0), matrix(width0 * height0, 0) {}

    template <typename O>
    friend std::ostream& operator<<(std::ostream& os, const Matrix<O>& m);

    void fill(T range);
    void resize(int new_height, int new_width, T value = 0);
};

template <typename T>
void Matrix<T>::fill(T range) {
    std::mt19937 engine(std::random_device{}()); // Seeded Mersenne Twister engine

    // Declare the distribution variable
    auto dist = [&]() {
        if constexpr (std::is_integral_v<T>) {
            return std::uniform_int_distribution<T>(0, range); // For integers
        } else if constexpr (std::is_floating_point_v<T>) {
            return std::uniform_real_distribution<T>(0.0, range); // For floating points
        } else {
            std::cerr << "The random generator function fill() cannot work with this type." << std::endl;
            exit(EXIT_FAILURE);
        }
    }(); // Immediately invoked lambda to create the correct distribution object

    auto generate_random_number = [&]() { return dist(engine); }; // Lambda to generate numbers
    std::generate(std::begin(matrix), std::end(matrix), generate_random_number); // Fill the matrix with random values
}

template <typename T>
void Matrix<T>::resize(int new_height, int new_width, T value){
    height = new_height;
    width = new_width;
    matrix.resize(new_width * new_height, value);
}

template <typename O>
std::ostream& operator<<(std::ostream& os, const Matrix<O>& m) {
    for (int i = 0; i < m.height; i++) {
        for (int j = 0; j < m.width; j++) {
            os << m.matrix[i * m.width + j] << " ";
        }
        os << std::endl;
    }
    return os;
}

int global_row(int local_row, int rank, int world_size, int N) {
    int rows_per_process = N / world_size;
    int extra_rows = N % world_size;
    int offset = rank * rows_per_process + std::min(rank, extra_rows);
    return local_row + offset;
}

template <typename T>
Matrix<T> generate_identity(int N, int rank = 0, int world_size = 1) {
    int rows_per_process = N / world_size;
    int extra_rows = N % world_size;

    int local_height = rows_per_process + (rank < extra_rows ? 1 : 0);
    int width = N;

    Matrix<T> identity(width, local_height);

    for (int i = 0; i < local_height; i++) {
        int global_i = global_row(i, rank, world_size, N);
        for (int j = 0; j < width; j++) {
            identity.matrix[i * width + j] = (global_i == j) ? 1 : 0;
        }
    }
    return identity;
}

template <typename T>
void print_in_parallel(const Matrix<T>& local_matrix, int rank, int world_size) {
    int width = local_matrix.width;
    if (width < 16) {
        int local_height = local_matrix.height;
        MPI_Datatype mpi_type = get_mpi_datatype<T>();

        if (rank == 0) {
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
                MPI_Recv(&source_local_height, 1, MPI_INT, source_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Receive and print each row
                for (int i = 0; i < source_local_height; ++i) {
                    std::vector<T> row_data(width);
                    MPI_Recv(row_data.data(), width, mpi_type, source_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
                MPI_Send(&local_matrix.matrix[local_row * width], width, mpi_type, 0, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        // For width >= 16, write to binary file
        MPI_Datatype mpi_type = get_mpi_datatype<T>();
        MPI_File fh;
        MPI_File_open(MPI_COMM_WORLD, "matrix.bin", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

        // Calculate offset in the file
        int rows_per_process = width / world_size;
        int extra_rows = width % world_size;
        int offset_rows = 0;

        for (int i = 0; i < rank; i++) {
            offset_rows += rows_per_process + (i < extra_rows ? 1 : 0);
        }

        MPI_Offset file_offset = static_cast<MPI_Offset>(offset_rows * width * sizeof(T));

        MPI_File_write_at_all(fh, file_offset, local_matrix.matrix.data(), local_matrix.height * width, mpi_type, MPI_STATUS_IGNORE);

        MPI_File_close(&fh);
    }
}

template <typename T>
void multiply_in_parallel(Matrix<T>& A, Matrix<T>& B, Matrix<T>& C, int N, int N_loc, int Np, const std::vector<int>& row_offsets, const std::vector<int>& N_locs) {
    // Create the vertical stripe of B (size N x N_loc)
    Matrix<T> B_stripe(N, N_loc);

    // Loop over each vertical stripe of B
    for (int stripe = 0; stripe < Np; ++stripe) {
        int col_start = row_offsets[stripe]; // Starting column index for process stripe
        int col_count = N_locs[stripe];      // Number of columns assigned to process stripe
        
        {
            SimpleTimer t("Comunication");
            // Resize the vertical stripe of B (size N x col_count)
            B_stripe.resize(N, col_count);

            // Each process extracts the columns [col_start, col_start + col_count) from its B_local
            // Pack data into send buffer
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
                recvcounts[p] = N_locs[p] * col_count; // Each process sends N_locs[p] x col_count elements
                displs[p] = total_recvcount;
                total_recvcount += recvcounts[p];
            }

            // Receive buffer to collect vertical stripe of B
            std::vector<T> recvbuf(total_recvcount);

            // Allgather the necessary columns to reconstruct the vertical stripe of B
            mpi_allgatherv<T>(sendbuf, N_loc * col_count, recvbuf, recvcounts, displs, MPI_COMM_WORLD);

            // Reconstruct the vertical stripe of B (size N x col_count)
            int offset = 0;
            for (int p = 0; p < Np; ++p) {
                int rows_p = N_locs[p];
                for (int i = 0; i < rows_p; ++i) {
                    int global_row = row_offsets[p] + i;
                    for (int j = 0; j < col_count; ++j) {
                        B_stripe.matrix[global_row * col_count + j] = recvbuf[offset + i * col_count + j];
                    }
                }
                offset += rows_p * col_count;
            }
        }
        {
            SimpleTimer t("Computation");
            // Multiply A_local (N_loc x N) with B_stripe (N x col_count) and store directly into C
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
void multiply_in_parallel_dgemm(Matrix<T>& A, Matrix<T>& B, Matrix<T>& C, int N, int N_loc, int Np, const std::vector<int>& row_offsets, const std::vector<int>& N_locs) {
    // Create the vertical stripe of B (size N x N_loc)
    Matrix<T> B_stripe(N, N_loc);

    // Loop over each vertical stripe of B
    for (int stripe = 0; stripe < Np; ++stripe) {
        int col_start = row_offsets[stripe]; // Starting column index for process stripe
        int col_count = N_locs[stripe];      // Number of columns assigned to process stripe
        
        {
            SimpleTimer t("DGEMM-Comunication");
            // Resize the vertical stripe of B (size N x col_count)
            B_stripe.resize(N, col_count);

            // Each process extracts the columns [col_start, col_start + col_count) from its B_local
            // Pack data into send buffer
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
                recvcounts[p] = N_locs[p] * col_count; // Each process sends N_locs[p] x col_count elements
                displs[p] = total_recvcount;
                total_recvcount += recvcounts[p];
            }

            // Receive buffer to collect vertical stripe of B
            std::vector<T> recvbuf(total_recvcount);

            // Allgather the necessary columns to reconstruct the vertical stripe of B
            mpi_allgatherv<T>(sendbuf, N_loc * col_count, recvbuf, recvcounts, displs, MPI_COMM_WORLD);

            // Reconstruct the vertical stripe of B (size N x col_count)
            int offset = 0;
            for (int p = 0; p < Np; ++p) {
                int rows_p = N_locs[p];
                for (int i = 0; i < rows_p; ++i) {
                    int global_row = row_offsets[p] + i;
                    for (int j = 0; j < col_count; ++j) {
                        B_stripe.matrix[global_row * col_count + j] = recvbuf[offset + i * col_count + j];
                    }
                }
                offset += rows_p * col_count;
            }
        }
        {
            SimpleTimer t("DGEMM-Computation");
            // Multiply A_local (N_loc x N) with B_stripe (N x col_count) and store directly into C
            cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        N_loc,              // Number of rows of A_local and C_sub
                        col_count,          // Number of columns of B_stripe and C_sub
                        N,                  // Number of columns of A_local and rows of B_stripe
                        1.0,                // Alpha coefficient
                        A.matrix.data(),    // A_local data
                        N,                  // Leading dimension of A_local
                        B_stripe.matrix.data(), // B_stripe data
                        col_count,          // Leading dimension of B_stripe
                        0.0,                // Beta coefficient
                        &C.matrix[col_start],  // C_sub data (start at column col_start)
                        N                   // Leading dimension of C (width of C)
            );
        }
        
    }
}


#endif
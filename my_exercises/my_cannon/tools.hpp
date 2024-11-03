#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <mpi.h>
#include <omp.h>
#include <random>
#include <type_traits>
#include "Parallel_Timer.hpp"
#include <cblas.h> // use -lblas to compile time 

// Función auxiliar para obtener el tipo de datos de MPI correspondiente al tipo T
template <typename T>
MPI_Datatype mpi_type();

template<>
MPI_Datatype mpi_type<int>() {
    return MPI_INT;
}

template<>
MPI_Datatype mpi_type<double>() {
    return MPI_DOUBLE;
}

template<>
MPI_Datatype mpi_type<float>() {
    return MPI_FLOAT;
}

// Clase CMatrix para representar una matriz de tamaño N x N y llenarla con valores aleatorios
template <typename T>
class CMatrix {
public:
    std::vector<T> matrix;
    size_t N;
    CMatrix(size_t N0) : N(N0), matrix(N0 * N0, T{}) {}
    void fill(T max0_to);
    T* data() { return matrix.data(); }
};

template <typename T>
void CMatrix<T>::fill(T max0_to) {
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
        std::cerr << "Type not supported: only int, double, and float." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
void multiply_blocks(const CMatrix<T>& A, const CMatrix<T>& B, CMatrix<T>& C, int BLOCK_SIZE) {
    #pragma omp parallel for
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        for (int j = 0; j < BLOCK_SIZE; ++j) {
            for (int k = 0; k < BLOCK_SIZE; ++k) {
                C.matrix[i * BLOCK_SIZE + j] += A.matrix[i * BLOCK_SIZE + k] * B.matrix[k * BLOCK_SIZE + j];
            }
        }
    }
}
template <typename T>
void multiply_blocks_blas(const CMatrix<T>& A, const CMatrix<T>& B, CMatrix<T>& C, int BLOCK_SIZE) {
    static_assert(std::is_same<T, double>::value, "cblas_dgemm requires double data type");

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE,
                1.0,
                A.matrix.data(), BLOCK_SIZE,   // Matriz A y su lda
                B.matrix.data(), BLOCK_SIZE,   // Matriz B y su ldb
                1.0,
                C.matrix.data(), BLOCK_SIZE);  // Matriz C y su ldc
}
template <typename T>
void gather_and_print_matrix(const CMatrix<T>& local_matrix, int BLOCK_SIZE, int N, int rank, int q, MPI_Comm comm, const std::string& matrix_name) {
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

    MPI_Gatherv(local_matrix.matrix.data(), size, mpi_type<T>(), full_matrix.data(), recv_counts.data(), displs.data(), mpi_type<T>(), 0, comm);

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

#endif // TOOLS_HPP


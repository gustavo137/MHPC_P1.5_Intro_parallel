#include <mpi.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <omp.h>

template <typename T> class CMesh {
public:
  std::vector<T> matrix;
  std::vector<T> new_matrix;
  size_t dim;
  int rank, size;
  
  CMesh(size_t &dim, std::vector<T> conini);
  void printf();
  void printnf();
  void apply_conditions(const std::vector<T> &conini);
}; // end class CMesh

template <typename T> class CSolver {
public:
  CMesh<T> M;
  CSolver(const CMesh<T> &M0) : M(M0) {}
  void jacobi(CMesh<T> &M, const size_t &ite, const size_t &pI);
  void savegnuplot(std::vector<double> &m, size_t dimm, size_t itt);
  void evolve(std::vector<double> &mat, std::vector<double> &new_mat, size_t dimm);
};

// Constructor de CMesh con inicialización de MPI
template <typename U>
CMesh<U>::CMesh(size_t &dim, std::vector<U> conini)
    : dim(dim), matrix(dim * dim), new_matrix((dim + 2) * (dim + 2)) {
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  for (size_t i = 0; i < dim * dim; ++i) {
    matrix[i] = 0.5;
  }
  apply_conditions(conini);
}

template <typename U>
void CSolver<U>::jacobi(CMesh<U> &M, const size_t &ite, const size_t &pI) {
  size_t rows_per_process = M.dim / M.size;
  size_t start_row = M.rank * rows_per_process;
  size_t end_row = start_row + rows_per_process - 1;

  for (size_t it = 0; it <= ite; it++) {
    evolve(M.matrix, M.new_matrix, M.dim);

    if (M.rank > 0) {
      MPI_Send(&M.new_matrix[start_row * M.dim], M.dim, MPI_DOUBLE, M.rank - 1, 0, MPI_COMM_WORLD);
      MPI_Recv(&M.new_matrix[(start_row - 1) * M.dim], M.dim, MPI_DOUBLE, M.rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (M.rank < M.size - 1) {
      MPI_Recv(&M.new_matrix[(end_row + 1) * M.dim], M.dim, MPI_DOUBLE, M.rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(&M.new_matrix[end_row * M.dim], M.dim, MPI_DOUBLE, M.rank + 1, 0, MPI_COMM_WORLD);
    }
    
    M.matrix.swap(M.new_matrix);

#ifdef PRINT
    if (it % pI == 0 && M.rank == 0) {
      savegnuplot(M.matrix, M.dim, it);
    }
#endif
  }

  if (M.rank == 0) {
    MPI_Gather(M.matrix.data(), rows_per_process * M.dim, MPI_DOUBLE, nullptr, 0, MPI_COMM_WORLD);
  } else {
    MPI_Gather(M.matrix.data() + start_row * M.dim, rows_per_process * M.dim, MPI_DOUBLE, nullptr, 0, MPI_COMM_WORLD);
  }
}

// Función evolve adaptada para MPI y OpenMP
template <typename T>
void CSolver<T>::evolve(std::vector<double> &mat, std::vector<double> &new_mat, size_t dimm) {
  size_t rows_per_process = dimm / M.size;
  size_t start_row = M.rank * rows_per_process + 1;
  size_t end_row = start_row + rows_per_process - 1;

#pragma omp parallel for
  for (size_t i = start_row; i <= end_row; ++i) {
    for (size_t j = 1; j <= dimm; ++j) {
      new_mat[i * (dimm + 2) + j] = 0.25 * (mat[(i - 1) * (dimm + 2) + j] + mat[i * (dimm + 2) + (j + 1)] +
                                            mat[(i + 1) * (dimm + 2) + j] + mat[i * (dimm + 2) + (j - 1)]);
    }
  }
}


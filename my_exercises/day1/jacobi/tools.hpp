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
  CMesh(size_t &dim, std::vector<T> conini);
  void printf();
  void printnf();
  void apply_conditions(const std::vector<T> &conini);
}; // end class CMesh

//// class Solver
/////////////////////////////
template <typename T> class CSolver {
public:
  CMesh<T> M;
  CSolver(const CMesh<T> &M0) : M(M0) {}
  void jacobi(CMesh<T> &M, const size_t &ite, const size_t &pI);
  // Functions needed for jacobi
  void savegnuplot(std::vector<double> &m, size_t dimm, size_t itt);
  void evolve(std::vector<double> &mat, std::vector<double> &new_mat,
              size_t dimm);
};

///////////// end class Solver
template <typename U>
void CSolver<U>::jacobi(CMesh<U> &M, const size_t &ite, const size_t &pI) {
  // M.printf(); // working, M.dim
  // std::cout<<ite<<std::endl; working
  M.matrix = M.new_matrix;

  // solve the jacobi
  for (size_t it = 0; it <= ite; it++) {
    evolve(M.matrix, M.new_matrix, M.dim);
    // swap the matrices
    M.matrix.swap(M.new_matrix);
    // to does not run this part. To run use -DPRINT
#ifdef PRINT
    // correct it % pI==0
    // to see if openmp is working I only print the it ==10 for example
    if (it == 0) { // correct it % pI ==0
      savegnuplot(M.matrix, M.dim, it);
    }
#endif
  }
  //

} //////// end of jacobi //////////////////////
////  Constructor of class CMesh
template <typename U>
CMesh<U>::CMesh(size_t &dim, std::vector<U> conini)
    : dim(dim), matrix(dim * dim), new_matrix((dim + 2) * (dim + 2)) {
  for (size_t i = 0; i < dim * dim; ++i) {
    matrix[i] = 0.5;
  }
  // fill the new_field with initial conditions
  apply_conditions(conini);
} // end constructor

// apply_conditions  to new_field
template <typename T>
void CMesh<T>::apply_conditions(const std::vector<T> &conini) {
  T end = conini[1];
  T star = conini[0];
  // The increment
  T dt = (end - star) / (T)((dim + 2) - 1);
  // fill the borders of new_field with increment
  for (size_t i = 0; i < (dim + 2); ++i) {
    new_matrix[((dim + 2) - 1) * (dim + 2) + i] =
        end - dt * i;                          // row down start -dt*i
    new_matrix[i * (dim + 2)] = star + dt * i; // column left
  }
  // fill the center of the matrix with field
  for (size_t i = 1; i <= dim; ++i) {
    for (size_t j = 1; j <= dim; ++j) {
      new_matrix[i * (dim + 2) + j] = matrix[(i - 1) * dim + (j - 1)];
    }
  }
}

// end conditions

// printf()
template <typename U> void CMesh<U>::printf() {
  for (size_t i = 0; i < dim; i++) {
    for (size_t j = 0; j < dim; j++) {
      std::cout << matrix[dim * i + j] << " ";
    }
    std::cout << std::endl;
  }
}
// end printf()
// printnf()
template <typename U> void CMesh<U>::printnf() {
  for (size_t i = 0; i < (dim + 2); i++) {
    for (size_t j = 0; j < (dim + 2); j++) {
      std::cout << new_matrix[(dim + 2) * i + j] << " ";
    }
    std::cout << std::endl;
  }
}
// end printf()
//
///////////////functions needed//////////////////////
template <typename T>
void CSolver<T>::savegnuplot(std::vector<double> &m, size_t dimm, size_t itt) {
  std::ostringstream oss;
  // oss << std::setw(2) << std::setfill('0') << i << ".txt";
  oss << itt << ".datjacobi";
  std::string filename = oss.str();
  std::ofstream file(filename);
  std::ifstream file2(filename);
  // file.close();
  if (file2) {
    // Imprimir el resultado en el archivo
    for (size_t i = 0; i < (dimm + 2); ++i) {
      for (size_t j = 0; j < (dimm + 2); ++j) {
        file << m[i * (dimm + 2) + j] << " ";
      }
      file << std::endl;
    }
    file.close();
  }
}
template <typename T>
void CSolver<T>::evolve(std::vector<double> &mat, std::vector<double> &new_mat,
                        size_t dimm) {
  // This will be a row dominant program.
  // para usar openmp
#pragma omp parallel for
  for (size_t i = 1; i <= dimm; ++i) {
    for (size_t j = 1; j <= dimm; ++j) {
      new_mat[(i * (dimm + 2)) + j] =
          (0.25) *
          (mat[((i - 1) * (dimm + 2)) + j] + mat[(i * (dimm + 2)) + (j + 1)] +
           mat[((i + 1) * (dimm + 2)) + j] + mat[(i * (dimm + 2)) + (j - 1)]);
    }
  }
}

////////////////end functions needed////////////////////

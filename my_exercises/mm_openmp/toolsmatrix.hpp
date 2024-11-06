#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <omp.h>

template <typename T>
class CMatrix {
public:
  std::vector<T> data;
  size_t size;
  CMatrix(const int &N);//:size(N),data(N*N){};
  //~CMatrix();
  void fill(); // function fill
  // new operators
  template <typename U>
  friend std::ostream &operator<<(std::ostream &os, const CMatrix<U> &p);
  template <typename U>
  friend CMatrix<U> operator*(const CMatrix<U> &A, const CMatrix<U> &B);
  template <typename U>
  friend CMatrix<U> operator+(const CMatrix<U> &A, const CMatrix<U> &B);
};

////////
// Constructor
template <typename T>
CMatrix<T>::CMatrix(const int &N) : size(N), data(N * N) {
//CMatrix<T>::CMatrix(const int &N){
    //data(N*N);
    std::vector<T> data;
   // for (size_t i = 0; i < N * N; i++) {
        //data[i] = 0;
        //data.push_back(0);
    //}
    //std::cout << "constructor called" << std::endl;
}
///////

// Destructor no more needed

template <typename U> 
void CMatrix<U>::fill() {
  for (size_t i = 0; i < size; i++) {
    for (size_t j = 0; j < size; j++) {
      size_t val = std::rand()%10;
      data[size*i+j]=val;
    }
  }
}
/////////
// Sobrecarga del operador << para imprimir la matriz
template <typename U>
std::ostream &operator<<(std::ostream &os, const CMatrix<U> &p) {
    for (size_t i = 0; i < p.size; i++) {
        for (size_t j = 0; j < p.size; j++) {
            os << p.data[p.size * i + j] << " ";
        }
        os << std::endl;
    }
    return os;
}
//////////////

template <typename U>
CMatrix<U> operator*(const CMatrix<U> &A, const CMatrix<U> &B) {
  CMatrix<U> prod(A.size); 
  // we only can use collapse(1) or collapse(2)
  #pragma omp parallel for collapse(1)
  for (size_t i = 0; i < prod.size; i++) {
    for (size_t j = 0; j < prod.size; j++) {
      prod.data[prod.size * i + j] = 0;
      for (size_t k = 0; k < prod.size; k++) {
        prod.data[prod.size * i + j] +=
            A.data[prod.size * i + k] * B.data[prod.size * k + j];
      }
    }
  }
  return prod;
}
template <typename U>
CMatrix<U> operator+(const CMatrix<U> &A, const CMatrix<U> &B) {
  CMatrix<U> sum(A.size);
  for (size_t i = 0; i < sum.size; i++) {
    for (size_t j = 0; j < sum.size; j++) {
      sum.data[sum.size * i + j] =
          A.data[sum.size * i + j] + B.data[sum.size * i + j];
    }
  }
  return sum;
}


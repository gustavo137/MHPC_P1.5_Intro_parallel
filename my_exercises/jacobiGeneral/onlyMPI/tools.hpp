#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <mpi.h>
#include "Parallel_Timer.hpp"
//#include <omp.h>

template <typename T> class CMesh {
public:
  std::vector<T> matrix;
  std::vector<T> new_matrix;
  size_t dim;
  size_t dim_local;
  size_t start_row;
  int rank;
  int world_size;
  CMesh(size_t &dim, size_t &dim_local, size_t &start_row, std::vector<T> conini, int rank, int world_size);
  void printf();
  void printnf();
  void apply_conditions(const std::vector<T> &conini);
  void print_in_parallel(std::ostream& out = std::cout);
  void print_in_serial();
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
  void evolve(std::vector<double> &mat, std::vector<double> &new_mat, size_t dimm, size_t dimm_local);

private:
  void exchange_ghost_rows(CMesh<T>& M);
};

///////////// end class Solver
template <typename U>
void CSolver<U>::jacobi(CMesh<U> &M, const size_t &ite, const size_t &pI) {
  // M.printf(); // working, M.dim
  // std::cout<<ite<<std::endl; working
  int rank = M.rank;
  int world_size = M.world_size;
  int dim = M.dim;
  int dim_local = M.dim_local;

  
  M.matrix = M.new_matrix;
  // Loop step by step 
  // solve the jacobi
  for (int it = 0; it <= ite; it++) {
    
    {Parallel_Timer t("Comunication_Time");
    // here is the function to exchange ghost 
    exchange_ghost_rows(M);
    }
    {Parallel_Timer t ("Calculation_Time");
    evolve(M.matrix, M.new_matrix, M.dim_local, M.dim);
    }
    // swap the matrices
    M.matrix.swap(M.new_matrix);
    // to does not run this part. To run use -DPRINT
    #ifdef PRINT
    // to print only one file to prub the results  it ==10 or 100
    if (it==10) { // correct it % pI ==0 to print all the files for the gif
      //only the root process print 
      if(rank==0){
         //savegnuplot(M.matrix, M.dim, it);
          std::ostringstream oss;
          // oss << std::setw(2) << std::setfill('0') << i << ".txt";
          oss << it << ".datjacobiMPI4";
          std::ofstream file(oss.str());
          if (file.is_open()) {
              // Imprimir el resultado en el archivo
              M.print_in_parallel(file);
             file.close();
          } else {
              std::cerr<<"Error opening the file for write: "<<std::endl;
          }
      }else{
        // others ranks participate in printing 
        M.print_in_parallel(std::cout);
      }
       
    }
    #endif
  }
  //

} //////// end of jacobi //////////////////////
////  Constructor of class CMesh
template <typename U>
CMesh<U>::CMesh(size_t &dim0, size_t &dim_local0, size_t &start_row0, std::vector<U> conini, int rank0, int world_size0)
    : dim(dim0), dim_local(dim_local0), start_row(start_row0), rank(rank0), world_size(world_size0){
  matrix.resize((dim_local+2)*(dim+2),0.5);
  new_matrix.resize((dim_local+2)*(dim+2),0.5);
  // fill the new_field with initial conditions
  apply_conditions(conini);
} // end constructor

// apply_conditions  to new_matrix based on rank
template <typename T>
void CMesh<T>::apply_conditions(const std::vector<T> &conini) {
  T end = conini[1];
  T star = conini[0];
  // The increment
  T dt = (end - star) / (T)((dim + 2) - 1); // step to increment bondary conditions
  
  //Apply boundary conditions based on rank
  if(rank==0){
    //top boundary (only for the first process)
    for(size_t j=0;j<dim+2;j++){
      new_matrix[0*(dim+2)+j]=0.0;//i =0 all time
    }
  }
  // bottom boundary (only for the last process)
  if(rank==world_size-1){
    for(size_t j=0;j< dim+2;j++){
     new_matrix[((dim_local+2)-1)*(dim+2)+j] = end -dt * j;
    }
  }
  // left and right boundaries for all process
  for(size_t i=0; i<dim_local+2;i++){
    size_t i_global = start_row + i;
    new_matrix[i*(dim+2)]=star+dt*i_global; //left
    new_matrix[i*(dim+2)+((dim+2)-1)]=0.0;  //right
  }
  
   //////////////////////////////////
}// end conditions

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
/// print in serial
template <typename U>
void CMesh<U>::print_in_serial(){
  for(size_t i =0; i < dim_local+2; i++){// to not include the gosh use i=1 <= dim_local
     for(size_t j = 0; j < dim+2; j++){
        std::cout << new_matrix[i*(dim+2)+j]<< " ";
     }
     std::cout<<std::endl;
  }
}
//end print in serial 
////////////////////////////////// print in parallel 
template <typename U>
void CMesh<U>::print_in_parallel(std::ostream& out){
    if (rank == 0) {
        // Si solo hay un procesador, imprime todo sin omitir filas
        if (world_size == 1) {
            for (size_t i = 0; i <= dim_local + 1; i++) {
                for (size_t j = 0; j < dim + 2; j++) {
                    out << new_matrix[i * (dim + 2) + j] << " ";
                }
                out << std::endl;
            }
        } else {
            // Para múltiples procesadores, proceso raíz imprime desde la primera fila hasta la última
            for (size_t i = 0; i <= dim_local; i++) {
                for (size_t j = 0; j < dim + 2; j++) {
                    out << new_matrix[i * (dim + 2) + j] << " ";
                }
                out << std::endl;
            }

            // Recibe y muestra los datos de los demás procesos
            int source_dim_local;
            int remainder = dim % world_size;

            for (int source_rank = 1; source_rank < world_size; ++source_rank) {
                source_dim_local = (source_rank < remainder) ? (dim / world_size + 1) : (dim / world_size);

                int end_row = (source_rank == world_size - 1) ? source_dim_local + 1 : source_dim_local;

                for (int i = 1; i <= end_row; ++i) {
                    std::vector<U> row_data(dim + 2);
                    MPI_Status status;
                    MPI_Recv(row_data.data(), dim + 2, MPI_DOUBLE, source_rank, 0, MPI_COMM_WORLD, &status);

                    for (size_t j = 0; j < dim + 2; j++) {
                        out << row_data[j] << " ";
                    }
                    out << std::endl;
                }
            }
        }
    } else {
        // Para procesos adicionales, envía sus filas adecuadamente
        int end_row = (rank == world_size - 1) ? dim_local + 1 : dim_local;

        for (int i = 1; i <= end_row; i++) {
            MPI_Send(&new_matrix[i * (dim + 2)], dim + 2, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }
}

////////////////////// end print in parallel
///////////////functions needed//////////////////////
template <typename T>
void CSolver<T>::savegnuplot(std::vector<double> &m, size_t dimm, size_t itt) {
  std::ostringstream oss;
  // oss << std::setw(2) << std::setfill('0') << i << ".txt";
  oss << itt << ".datjacobi";
  std::ofstream file(oss.str());
  if (file.is_open()) {
    // Imprimir el resultado en el archivo
    for (size_t i = 0; i < (dimm + 2); ++i) {
        for (size_t j = 0; j < (dimm + 2); ++j) {
            file << m[i * (dimm + 2) + j] << " ";
        }
        file << std::endl;
    }
    file.close();
  } else {
    std::cerr<<"Error opening the file for write: "<<std::endl;
  }
}
template <typename T>
void CSolver<T>::evolve(std::vector<double> &mat, std::vector<double> &new_mat,
                        size_t dimm_local, size_t dimm) {
  // This will be a row dominant program.
  // para usar openmp
//#pragma omp parallel for
  for (size_t i = 1; i <= dimm_local; ++i) {
    for (size_t j = 1; j <= dimm; ++j) {
      new_mat[(i * (dimm + 2)) + j] =
          (0.25) *
          (mat[((i - 1) * (dimm + 2)) + j] + mat[(i * (dimm + 2)) + (j + 1)] +
           mat[((i + 1) * (dimm + 2)) + j] + mat[(i * (dimm + 2)) + (j - 1)]);
    }
  }
}
// evolve ends
//////// function exchange_ghost_rows(Mesh<T>& m);
template <typename T>
void CSolver<T>::exchange_ghost_rows(CMesh<T>& M){
  int dim=M.dim;
  int dim_local = M.dim_local;
  int rank = M.rank;
  int world_size = M.world_size;
  
  int up_rank = (rank==0) ? MPI_PROC_NULL : rank - 1;
  int down_rank = (rank==world_size -1) ? MPI_PROC_NULL : rank + 1;
  
  // send and recive buffers
  std::vector<T> send_to_up(dim+2);
  std::vector<T> recv_from_down(dim+2);
  std::vector<T> send_to_down(dim+2);
  std::vector<T> recv_from_up(dim+2);
 
  // Copy data to send buffers
  std::copy(&M.matrix[(1) * (dim + 2)], &M.matrix[(2) * (dim + 2)], send_to_up.begin());
  std::copy(&M.matrix[(dim_local) * (dim + 2)], &M.matrix[(dim_local + 1) * (dim + 2)], send_to_down.begin());
 
  // Exchange with up_rank and down_rank
  mpi_sendrecv(send_to_up, up_rank, 0, recv_from_down, down_rank, 0, MPI_COMM_WORLD);
  mpi_sendrecv(send_to_down, down_rank, 1, recv_from_up, up_rank, 1, MPI_COMM_WORLD);

  // Copy received data into ghost rows
  if (down_rank != MPI_PROC_NULL) {
     std::copy(recv_from_down.begin(), recv_from_down.end(), &M.matrix[(dim_local + 1) * (dim + 2)]);
  }
  if (up_rank != MPI_PROC_NULL) {
     std::copy(recv_from_up.begin(), recv_from_up.end(), &M.matrix[0]);
  }

}
/// end void exchange_ghost_rows
////////////////end functions needed////////////////////

//////  functions MPI_ send recv
template<typename T>
void mpi_sendrecv(std::vector<T>& sendbuff, int dest, int sendtag, std::vector<T>& recvbuff, int source, int recvtag, MPI_Comm comm) {
      if constexpr (std::is_same<T, int>::value) {
          MPI_Sendrecv(sendbuff.data(), sendbuff.size(), MPI_INT, dest, sendtag,
                       recvbuff.data(), recvbuff.size(), MPI_INT, source, recvtag,
                       comm, MPI_STATUS_IGNORE);
      } else if constexpr (std::is_same<T, double>::value) {
          MPI_Sendrecv(sendbuff.data(), sendbuff.size(), MPI_DOUBLE, dest, sendtag,
                       recvbuff.data(), recvbuff.size(), MPI_DOUBLE, source, recvtag,
                       comm, MPI_STATUS_IGNORE);
      } else {
         MPI_Sendrecv(sendbuff.data(), sendbuff.size() * sizeof(T), MPI_BYTE, dest, sendtag,
                       recvbuff.data(), recvbuff.size() * sizeof(T), MPI_BYTE, source, recvtag,
                       comm, MPI_STATUS_IGNORE);
      }
}
/////////   end functions of MPI

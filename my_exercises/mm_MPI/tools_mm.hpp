#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <random>
#include <omp.h>
#include "mpi_tools.hpp"

template <typename T>
class CMatrix {
public:
  std::vector<T> mat;
  int height, width;
  CMatrix(int height0, int width0);//:size(N),data(N*N){};
  //~CMatrix();
  void fill(T max0_to); // function fill with diferents types from 0 to max0_to
  // new operators
  // check if Can I print using this function. Other with create a function print_in_parallel();
  template <typename U>
  friend std::ostream &operator<<(std::ostream &os, const CMatrix<U> &p);
  void matresize(int new_height, int new_width, T fill_with = 0);//this function change the value of height and width, new size con value "fill_with"
};

////////
// Constructor
template <typename T>
CMatrix<T>::CMatrix(int height0, int width0): height(height0), width(width0), mat(height0*width0,0){}
///////
///////// fill(start, end)
template <typename T>
void CMatrix<T>::fill(T max0_to) {
    if constexpr (std::is_same_v<int, T>) {
        std::mt19937 engine(std::random_device{}());
        std::uniform_int_distribution<int> dist(1, max0_to);
        auto generate_random_number = [&]() { return dist(engine); };
        // Llenar toda la matriz
        std::generate(mat.begin(), mat.end(), generate_random_number);
    }
    else if constexpr (std::is_same_v<double, T>) {
        std::mt19937 engine(std::random_device{}());
        std::uniform_real_distribution<double> dist(1.0, max0_to);
        auto generate_random_number = [&]() { return dist(engine); };
        std::generate(mat.begin(), mat.end(), generate_random_number);
    }
    else if constexpr (std::is_same_v<float, T>) {
        std::mt19937 engine(std::random_device{}());
        std::uniform_real_distribution<float> dist(1.0f, max0_to);
        auto generate_random_number = [&]() { return dist(engine); };
        std::generate(mat.begin(), mat.end(), generate_random_number);
    }
    else{
      std::cerr<<"Type no soported: only int, double and float:"<<std::endl;
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
    //os<<p.mat[0]<<" "<<p.mat[1]<<" "<<p.mat[2]<<" "<<p.mat[3]<<std::endl;
    return os;
}
////////////// en <<
///////////////////// matresize() /////
template <typename T>
void CMatrix<T>::matresize(int new_height, int new_width, T fill_with){
   height=new_height;
   width = new_width;
   mat.resize(new_height*new_width, fill_with);
}
//////////// end matresize()////////

////////////////////// function to calcule the global_row() ///
int global_row(int local_row, int rank, int npes, int N){
  int rows_per_process = N/npes;
  int extra_rows = N % npes;
  int offset = rank*rows_per_process + std::min(rank, extra_rows);// whats is offset
  return local_row + offset;
}
////// end global_row /// 

//////////// main function multiply_in_parallel //////////////////
template <typename T>
void multiply_in_parallel(CMatrix<T>& A, CMatrix<T>& B, CMatrix<T>& C, int N_loc,int N,int npes, std::vector<int> row_displacements, std::vector<int>N_locs){
   //std::cout<<C<<std::endl;
   // crear the vertical columna of B
   CMatrix<T> B_col(N, N_loc);
   // loop for every columna of B
   for(int col=0; col<npes;col++){
     int col_start =  row_displacements[col];//Starting colum index for process col
     int col_count = N_locs[col];    // number of colums asigned to process col

     /// start comunication 
     // resize the B_col vertical B_col(N,col_count)
     B_col.matresize(N, col_count);

     // now each proces extracts the columns [col_start, col_start + col_count];
     // so we need to pack the data to send in the buffer
     std::vector<T> buffer_to_send(N_loc * col_count);
     for(int i=0;i<N_loc; i++){
       for(int j=0;j<col_count;j++){
          buffer_to_send[i*col_count + j]=B.mat[i*N + col_start + j];
       }
     }
     
     // now prepare count and displacements to MPI_Gatherv
     std::vector<int> recv_counts(npes);
     std::vector<int> displacements(npes);
     int total_recv_count=0;
     for(int rank = 0;rank<npes;rank++){
       recv_counts[rank] = N_locs[rank]*col_count;// Each process sends N_locs[rank]x col_count elements
       displacements[rank] = total_recv_count;
       total_recv_count += recv_counts[rank];
     }
     
     // now recive buffer to vertical col of B
     std::vector<T> recv_buffer(total_recv_count);

     // now do all gather with th enecesary colums to reconstruct the vertical colum of B
     // remember this function is in mpi_tools.hpp
     mpi_allgatherv(buffer_to_send, N_loc*col_count,recv_buffer, recv_counts,displacements,MPI_COMM_WORLD);
     
     //Now we need to reconstruct the vertical col of B of size  (N x col_count)
     int displas = 0;//(offsets)
     for(int rank = 0; rank<npes;rank++){
        int rows_rank = N_locs[rank];
        for(int i=0;i<rows_rank;i++){
           int global_row = displacements[rank]+i;
           for(int j=0;j<col_count;j++){
              B_col.mat[global_row*col_count + j]=recv_buffer[displas + i*col_count + j];
           }
        }
        displas += rows_rank*col_count; 
     } 
     
     /// end comunication 
     
     /// start computation 
     // multiply A_loc (N_loc x N) with B_col(N x col_count) and store the result in C
     for(int i=0;i<N_loc;i++){
       for(int j=0;j<col_count;j++){
          T sum=0;
          for(int k=0;k<N;k++){
            sum += A.mat[i*N_loc+k] * B_col.mat[k*col_count+j];
          }
          // store in C[N_loc,col_count]
          C.mat[i*N + col_start + j]=sum;
       }
     } 
     /// end computation 
   } // end loop for every colum of B
}
//////////// end multiply in parallel ////////////////////////////

/////////// print in parallel ////////////////////
template <typename T>
void print_in_parallel(const CMatrix<T>& local_matrix, int rank, int world_size) {
     int width = local_matrix.width;
     if (width < 16) {
        int local_height = local_matrix.height;
         MPI_Datatype mpi_type = get_mpi_datatype<T>();
 
         if (rank == 0) {
             std::cout<<"matrix"<<std::endl;
             // Root process prints its own matrix
             for (int local_row = 0; local_row < local_height; ++local_row) {
                 // Print the row
                 for (int col = 0; col < width; ++col) {
                     std::cout << local_matrix.mat[local_row * width + col] << " ";
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
                 MPI_Send(&local_matrix.mat[local_row * width], width, mpi_type, 0, 0, MPI_COMM_WORLD);
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
 
         MPI_File_write_at_all(fh, file_offset, local_matrix.mat.data(), local_matrix.height * width, mpi_type, MPI_STATUS_IGNORE);
 
         MPI_File_close(&fh);
     }
}
////////////// end print in parallel /////////

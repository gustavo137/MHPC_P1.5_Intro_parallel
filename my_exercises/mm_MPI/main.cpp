/**
 * 
 * To do: replace multiply_in_parallel with DGEMM
 * Compare the scaling for N=5000 of the two cases
 * tor 1, 2, 4, 8, 16 nodes
 * 
 * compile it with
 * mpic++ -O3 -Iinclude main.cpp -o matrix_multiplication -lopenblas
 * run with
 * mpirun --oversubscribe -np 8 ./matrix_multiplication
 * 
 */
#include <iostream>
#include <mpi.h>
#include "matrix.hpp"
#include "simple_timer.hpp"


int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);

    int rank, Np;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Np);
    
    int N = 4;
    if (argc > 1) {
        N = std::stoi(argv[1]);
    }

    int remainder = N % Np;
    std::vector<int> N_locs(Np);
    std::vector<int> row_offsets(Np + 1, 0);
    for (int i = 0; i < Np; ++i) {
        N_locs[i] = (i < remainder) ? (N / Np + 1) : (N / Np);
        row_offsets[i + 1] = row_offsets[i] + N_locs[i];
    }

    int N_loc = N_locs[rank];
    int start_row = row_offsets[rank];

    Matrix<double> A(N_loc, N), B(N_loc, N), C(N_loc, N), D(N_loc, N);
    A.fill(1);
    B.fill(1);

    /*std::cout<< "A is: " << std::endl;
    print_in_parallel(A, rank, Np);
    std::cout<< "B is: " << std::endl;
    print_in_parallel(B, rank, Np);*/
 
    // Begin matrix multiplication
    {
        SimpleTimer t("Matrix Multiplication");
        multiply_in_parallel(A, B, C, N, N_loc, Np, row_offsets, N_locs);
    }
    {
        SimpleTimer t("DGEMM Matrix Multiplication");
        multiply_in_parallel_dgemm(A, B, D, N, N_loc, Np, row_offsets, N_locs);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /*std::cout<< "C is: " << std::endl;
    print_in_parallel(C, rank, Np);*/


    SimpleTimer::gather_timing_data(MPI_COMM_WORLD, 0);

    MPI_Finalize();
    return 0;
}

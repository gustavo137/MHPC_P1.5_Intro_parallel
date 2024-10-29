#include <fstream>
#include <iostream>
#include <mpi.h>
#include <vector>

// Basic function to print
void print_mat(const std::vector<double>& vec, int loc_size, int N) {
    for (int i = 0; i < loc_size; ++i) {
        for (int j = 0; j < N; j++) {
            std::cout << vec[i * N + j] << " ";
        }
        std::cout << std::endl;
    }
}

// Function to print every rank data
void print_mat_par(std::vector<double> mat, int n_loc, int N, int npes, int rank, int rest) {
    if (rank == 0) {
        std::cout << std::endl;
        print_mat(mat, n_loc, N);
        for (int count = 1; count < npes; count++) {
            int recv_size = (count < rest) ? (n_loc + 1) : n_loc; // determinar el tamaño correcto
            std::vector<double> recv_mat(recv_size * N);
            MPI_Recv(recv_mat.data(), recv_size * N, MPI_DOUBLE, count, count, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            print_mat(recv_mat, recv_size, N);
        }
    } else {
        MPI_Send(mat.data(), n_loc * N, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    // size of the matrix N*N
    int N{7};
    // MPI related variables
    int NPES = 1, RANK = 0;
    int N_LOC = N, i_GLOBAL = 0;
    int OFFSET = 0, REST = 0;
    // get the size of the world and rank
    MPI_Comm_size(MPI_COMM_WORLD, &NPES);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    // variables for NPES>1
    N_LOC = N / NPES;
    REST = N % NPES;

    if (RANK < REST) {
        N_LOC += 1; 
    }

    // Crear la matriz
    std::vector<double> Mat(N_LOC * N);

    // Cálculo del índice global
    i_GLOBAL = (N_LOC * RANK) + (RANK < REST ? RANK : REST);

    // Llenar la matriz
    for (int i_local = 0; i_local < N_LOC; ++i_local) {
        int i_global_actual = i_GLOBAL + i_local;
        for (int j = 0; j < N; ++j) {
            Mat[i_local * N + j] = (i_global_actual == j) ? 1.0 : 0.0;
        }
    }

    // Imprimir matrices
    print_mat_par(Mat, N_LOC, N, NPES, RANK, REST);

    MPI_Finalize();
    return 0;
}


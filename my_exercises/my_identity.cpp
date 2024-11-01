#include <fstream>
#include <iostream>
#include <mpi.h>
#include <vector>

void print_mat(const std::vector<double>& mat, int N_LOC, int N) {
    for (int i = 0; i < N_LOC; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << mat[i * N + j] << " ";
        }
        std::cout << std::endl;
    }
}

void print_mat_par(std::vector<double>& mat, int N_LOC, int N, int rank, int npes, int rest) {
    if (rank == 0) {
       // std::cout << std::endl;
        print_mat(mat, N_LOC, N);
        for (int count = 1; count < npes; ++count) {
            if(count==rest){N_LOC -=1;}
            MPI_Recv(mat.data(), N_LOC * N, MPI_DOUBLE, count, count, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            print_mat(mat, N_LOC, N);
        }
    } else {
        MPI_Send(mat.data(), N_LOC * N, MPI_DOUBLE, 0, rank, MPI_COMM_WORLD);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int NPES;
    MPI_Comm_size(MPI_COMM_WORLD, &NPES);
    int RANK;
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    int N = 5;
    int N_LOC = N / NPES;
    int REST = N % NPES;

    if (RANK < REST) N_LOC += 1;

    std::vector<double> Mat(N_LOC * N, 0.0);
    int OFFSET=0;

    //int OFFSET = (RANK < REST) ? RANK : REST;
    if (RANK < REST) {
      OFFSET = RANK;
    } else {
      OFFSET = REST;
    }

    int i_GLOBAL = (N / NPES) * RANK + OFFSET;

    for (int i_local = 0; i_local < N_LOC; ++i_local) {
        int i_global_actual = i_GLOBAL + i_local;
        Mat[i_local * N + i_global_actual] = 1.0;
    }

    print_mat_par(Mat, N_LOC, N, RANK, NPES, REST);

    MPI_Finalize();
    return 0;
}


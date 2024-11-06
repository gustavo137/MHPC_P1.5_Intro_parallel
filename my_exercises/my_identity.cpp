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
            if(count==rest){N_LOC -=1;} //solo se aplica al ultimo que solo tiene un renglon 
            // en esta funcion ponemos tag = count para saber quien envia, en este caso envia count.
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

    int N = 5; // size of the matrix by default 
    if(argc > 1){
       N=std::atoi(argv[1]); 
    }
  
    int N_LOC = N / NPES;// division entera, if NPES=3, N_LOC=1;
    int REST = N % NPES; // resto de la division entera 5%3 = 2; 
   
    //Solo a los primeros ranks se les agrega una fila mas de acuerdo a REST
    if (RANK < REST) N_LOC += 1; // RANK=0 has N_LOC=2, RANK=1 has N_LOC=2 and RANK=2 has N_LOC =1
    
    // creamos el vector para llenar la infomacion para cada RANK con su respectivo N_LOC * N que es el renglon.
    std::vector<double> Mat(N_LOC * N, 0.0); // se inicializa en cero dado que aqui solo lo creamos 
    int OFFSET=0;// este es el incremento 

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


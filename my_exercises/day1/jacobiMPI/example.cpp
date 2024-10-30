//To use the timer 
/**
*  MPI_Init(&argc, &argv);<----------------------------------------
*  USE THIS AS
*  {CParallel_Timer t("WHATEVER YOU ARE TIMING");
*        THE CODE YOU ARE TIMING
*    }
* 
*  THEN AT THE END OF MAIN PUT
*  CParallel_Timer::gather_and_process_times(world_rank, world_size); 
*  MPI_Finalize();<----------------------------------------  
*/ 

#include "Parallel_Timer.hpp"
#include <mpi.h>
#include <thread> // para usar std::this_thread::sleep_for

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Probar el temporizador con una función de sleep
    { 
        CParallel_Timer t("sleep_test");
        std::this_thread::sleep_for(std::chrono::seconds(1 + world_rank)); // Diferente tiempo de sleep por proceso
    }

    // Recolectar los resultados en el proceso raíz
    CParallel_Timer::gather_and_process_times(world_rank, world_size);

    MPI_Finalize();
    return 0;
}


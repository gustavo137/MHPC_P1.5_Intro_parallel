#!/bin/bash
#SBATCH --job-name=2_mpi_omp_jacobi    ## Job name
#SBATCH --output=omp_mpi_2N.datg
#SBATCH -N 2 # Number of nodes
#SBATCH --ntasks-per-node=32                   ## Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               ## Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running jacobiMPI N2 ibrid"
###export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK ### to use cpus-per-node
##module load gcc/12.2.0
##module load openmpi/4.1.6--gcc--12.2.0
##mpic++ -O3 main.cpp -o main.x -I. -fopenmp

## here we can only put : mpirun ./main.x   because the computer fills the np itself
## remember -np = N*(ntasks_per_node)
mpirun ./main.x 12000
##mpirun -np 128 ./main.x


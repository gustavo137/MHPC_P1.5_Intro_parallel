#!/bin/bash
#SBATCH --job-name=1_mpi_omp_jacobi    ## Job name
#SBATCH --output=1N.datg
#SBATCH -N 1 # Number of nodes
#SBATCH --ntasks-per-node=32                   ## Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               ## Time limit hrs:min:sec
#SBATCH -p regular2

echo "Running jacobiMPI N1 hybrid"
###export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK ### to use cpus-per-node
##module load gcc/12.2.0
##module load openmpi/4.1.6--gcc--12.2.0
## load in ulysses
##module load ulysses
##module load gnu11
##module load openmpi4
mpic++ -O3 main.cpp -o main.x -I. -fopenmp

## here we can only put : mpirun ./main.x   because the computer fills the np itself
## remember -np = N*(ntasks_per_node)
mpirun ./main.x 12000
##mpirun -np 128 ./main.x


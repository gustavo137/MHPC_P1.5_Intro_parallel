#!/bin/bash
#SBATCH --job-name=1_mpi_jacobi    # Job name
#SBATCH --output=jacobi_1N.datg
#SBATCH -N 1 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               # Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running jacobiMPI 1"
module load gcc
module load openmpi
mpic++ -O3 main.cpp -o main.x -I.

mpirun -np 32 ./main.x 12000

#!/bin/bash
#SBATCH --job-name=16_mpi_jacobi    # Job name
#SBATCH --output=jacobi_16N.datg
#SBATCH -N 16 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               # Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running jacobiMPI 16"
#module load openmpi/4.1.6--gcc--12.2.0
#mpic++ main.cpp -o main.x -I.

mpirun -np 512 ./main.x

#!/bin/bash
#SBATCH --job-name=64_mpi_jacobi    # Job name
#SBATCH --output=jacobi_64N.datg
#SBATCH -N 64 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:10:00               # Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running jacobiMPI 64"
#module load openmpi/4.1.6--gcc--12.2.0
#mpic++ main.cpp -o main.x -I.

mpirun -np 2048 ./main.x 12000

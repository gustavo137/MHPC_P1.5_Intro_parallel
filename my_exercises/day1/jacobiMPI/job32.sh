#!/bin/bash
#SBATCH --job-name=32_mpi_jacobi    # Job name
#SBATCH --output=jacobi_32N.dat
#SBATCH -N 32 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:10:00               # Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running jacobiMPI 32"
#module load openmpi/4.1.6--gcc--12.2.0
#mpic++ main.cpp -o main.x -I.

mpirun -np 1024 ./main.x

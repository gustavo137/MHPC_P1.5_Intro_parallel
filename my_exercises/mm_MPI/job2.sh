#!/bin/bash
#SBATCH --job-name=2_mpi_mm    # Job name
#SBATCH --output=mm_2N.datg
#SBATCH -N 2 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               # Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running mm N2"
module load gcc
module load openmpi
module load openblas
mpic++ -O3 main.cpp -o main.x -I. -lopenblas 

mpirun -np 64 ./main.x 5000

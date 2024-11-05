#!/bin/bash
#SBATCH --job-name=32_mpi_mm    # Job name
#SBATCH --output=mm_32N.datg
#SBATCH -N 32 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               # Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running mm N32"
module load gcc/12.2.0
module load openmpi/4.1.6--gcc--12.2.0
module load openblas
mpic++ -O3 main.cpp -o main.x -I. -lblas 

mpirun -np 1024 ./main.x 5000

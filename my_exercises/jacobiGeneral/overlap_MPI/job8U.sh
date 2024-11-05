#!/bin/bash
#SBATCH --job-name=8_overlap_jacobi    # Job name
#SBATCH --output=8NU.datg
#SBATCH -N 8 # Number of nodes
#SBATCH --ntasks-per-node=32                   # Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               # Time limit hrs:min:sec
#SBATCH -p regular2

echo "Running jacobi 8"
##module load gcc/12.2.0
##module load openmpi/4.1.6--gcc--12.2.0
##module load ulysses
##module load gnu11
##module openmpi4

##mpic++ -O3 main.cpp -o main.x -I. 

mpirun ./main.x 12000

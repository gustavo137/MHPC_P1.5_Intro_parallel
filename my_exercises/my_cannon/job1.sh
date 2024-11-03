#!/bin/bash
#SBATCH --job-name=1_mpi_blas_cannon    ## Job name
#SBATCH --output=1N.datg
#SBATCH -N 1 # Number of nodes
#SBATCH --ntasks-per-node=32                   ## Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               ## Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running cannon N1"
###export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK ### to use cpus-per-node
module load gcc/12.2.0
module load openmpi/4.1.6--gcc--12.2.0
module load openblas 
mpic++ -O3 main.cpp -o main.x -I. -lopenblas  ### -lblas

## here we can only put : mpirun ./main.x   because the computer fills the np itself
## remember -np = N*(ntasks_per_node)
mpirun -np 1 ./main.x 5000

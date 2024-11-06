#!/bin/bash
#SBATCH --job-name=4_mpi_blas_cannon    ## Job name
#SBATCH --output=4N.datg
#SBATCH -N 1 # Number of nodes
#SBATCH --ntasks-per-node=4                   ## Run on a single CPU
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00               ## Time limit hrs:min:sec
#SBATCH -A ICT24_MHPC
#SBATCH -p boost_usr_prod

echo "Running cannon N4"
###export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK ### to use cpus-per-node
module load gcc
module load openmpi
module load openblas
mpic++ -O3 main.cpp -o main.x -I. -lopenblas

## here we can only put : mpirun ./main.x   because the computer fills the np itself
## remember -np = N*(ntasks_per_node)
##mpirun -np 4 ./main.x 5000
mpirun -np 4 ./main.x 5000

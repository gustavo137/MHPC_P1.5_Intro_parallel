#!/bin/bash
#SBATCH --job-name=my_fft_v2                # Job name
#SBATCH --output=my_fft_v2.datg               # output
###SBATCH --mail-type=END,FAIL             # Mail events (NONE, BEGIN, END, FAIL, ALL)
###SBATCH --mail-user=gparedes@ictp.it     # Where to send mail        
#SBATCH -N 1                               # Number of nodes only use 1 for this exercise
#SBATCH --ntasks-per-node=20                # Run on a single CPU, only change this until 24 I believe 
#SBATCH --time=00:25:00                    # Time limit hrs:min:sec
#SBATCH --cpus-per-task=1
##SBATCH --mem=0                           # with 0 take all the memory available
#SBATCH --partition=regular1               #
##SBATCH --reservation=mhpc_2024_11         ## for more than 4 nodes comment this line

echo "fast Fourier Transform N1"

module load gnu11/11.2.1
module load openmpi3/3.1.4
module load fftw/3.3.8

## make ## I dont know if this works 

##mpirun ./diffusion.x >> out.dat
echo "np 1" 
mpirun -np 1 ./diffusion.x

echo "np 2" 
mpirun -np 2 ./diffusion.x

echo "np 4" 
mpirun -np 4 ./diffusion.x

echo "np 8" 
mpirun -np 8 ./diffusion.x

echo "np 16" 
mpirun -np 16 ./diffusion.x

echo "np 20" 
mpirun -np 20 ./diffusion.x

echo "finished"

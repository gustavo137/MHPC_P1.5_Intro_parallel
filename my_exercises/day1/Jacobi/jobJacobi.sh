#!/bin/bash
#SBATCH -A ICT24_MHPC                    #  <account_name>
#SBATCH --job-name=Jacobi_openmp       # Job name
#SBATCH --mail-type=END,FAIL             # Mail events (NONE, BEGIN, END, FAIL, ALL)
#SBATCH --mail-user=gparedes@ictp.it     # Where to send mail
#SBATCH -N 1                             # Number of nodes
##SBATCH --ntasks-per-node=1             # Run on a single CPU
#SBATCH --cpus-per-task=32               # 2, 4, 8, 16, 32
#SBATCH --time=00:10:00                  # Time limit hrs:min:sec
#SBATCH -p boost_usr_prod                # for smaller works use --qos=boost_qos_dbg ,     boost is dedicated to works with GPU


echo "Running jacobi using openmp for size=10000"
##module load nvhpc/24.3 
##gcc -stdpar=multicore main.cpp -o main.x -I.
g++ main.cpp -o main.x -I. -fopenmp
export "num threads = 4"
export OMP_NUM_THREADS=4
./main.x

export "num threads = 8"
export OMP_NUM_THREADS=8
./main.x

export "num threads = 12"
export OMP_NUM_THREADS=12
./main.x

export "num threads = 16"
export OMP_NUM_THREADS=16
./main.x

export "num threads = 20"
export OMP_NUM_THREADS=20
./main.x

export "num threads = 24"
export OMP_NUM_THREADS=24
./main.x

export "num threads = 28"
export OMP_NUM_THREADS=28
./main.x

export "num threads = 32"
export OMP_NUM_THREADS=32
./main.x

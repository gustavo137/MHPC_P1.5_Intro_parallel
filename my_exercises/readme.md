# This is a folder for my exercises
 
## run in ulysses

To run in ulyses we need to load 

module load ulysses

module load openmpi4

module load gnu11

module openblas
 
and for run a code using blas we need to add the next flags

-I$OPENBLAS_ING

-L$OPENBLAS_LIB

-lopenblas 

and maybe 

-std=c++17



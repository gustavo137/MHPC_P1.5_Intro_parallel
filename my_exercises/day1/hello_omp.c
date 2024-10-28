#include <stdlib.h>
#include <stdio.h>

#include <omp.h>

//gcc main.c -o main.x -fopenmp 
// check using: export OMP_NUM_THREADS=4

int main(int argc, char * argv[]){
 
  int th_id, num_threads; 

 #pragma omp parallel 
{
 int num_ths = omp_get_num_threads();
 int id = omp_get_thread_num();
 if(!id) printf("Running with %d threads", num_ths);
}

 #pragma omp parallel private(th_id,num_threads)
 {
  #ifdef __DEBUG
   th_id = omp_get_thread_num(); 
   num_threads = omp_get_num_threads();

   //std::cout<<"Hello I am in thread  "<<t_id<<" from "<< num_threads<<std::endl;
   if(th_id) printf("\n Hello I am th_id = %d, out of %d threads",th_id,num_threads);
  #endif 
 }
  
  return 0;
}

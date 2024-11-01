# Results

With n=1000, when I increase the number of threads the time increases, 
so in order to have good results I need to increase n, for this example I use n=1000000, 
the results are:

**reduction(+:sum)** time with 8 threads was 2505 micro seconds

**atomic** time with 8 threads was 227531 micro seconds 

**critical** time with 8 threads was 809340 micro seconds, 
 
so using reduction is faster. 

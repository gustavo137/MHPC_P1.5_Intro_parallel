#!/bin/bash
#SBATCH -A ICT24_MHPC                     # <account_name>
#SBATCH --job-name=Jacobi_openmp          # Job name
#SBATCH --mail-type=END,FAIL              # Mail events (NONE, BEGIN, END, FAIL, ALL)
#SBATCH --mail-user=gparedes@ictp.it      # Where to send mail
#SBATCH -N 1                              # Number of nodes
#SBATCH --cpus-per-task=32                # Maximum number of CPUs to test with
#SBATCH --time=00:10:00                   # Time limit hrs:min:sec
#SBATCH -p boost_usr_prod                 # Use appropriate partition

# Eliminar el archivo de resultados anterior si existe
rm -f results_leo2.txt

# Compilar el programa con OpenMP
g++ -O3 main.cpp -o main.x -I. -fopenmp

# Iterar sobre diferentes cantidades de threads (en este caso de 1 a 32)
for num_threads in {1..120}; do
    echo "Running with OMP_NUM_THREADS=$num_threads"
    export OMP_NUM_THREADS=$num_threads

    # Ejecutar el programa y capturar su salida
    output=$(./main.x)

    # Extraer la línea que contiene "Duration for 'Jacobi using openmp'"
    line=$(echo "$output" | grep "Duration for 'Jacobi using openmp'")

    # Verificar si la línea no está vacía
    if [ -n "$line" ]; then
        # Extraer el tiempo en microsegundos de la línea
        duration_micro=$(echo "$line" | grep -oP "Duration for 'Jacobi using openmp': \K[0-9]+(?= micro s)")
        # Guardar el número de threads y el tiempo en microsegundos en results.txt
        echo "$num_threads $duration_micro" >> results_leo2.txt
    else
        # Si no se encuentra el tiempo, guardar 0 como tiempo
        echo "$num_threads 0" >> results_leo2.txt
    fi
done


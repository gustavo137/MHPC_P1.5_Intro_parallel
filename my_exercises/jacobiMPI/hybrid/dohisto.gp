set terminal pngcairo
set output 'grafico.png'

# Cargar los datos
set style data histogram
set style histogram rowstacked
set boxwidth 0.5
set grid y

set ylabel "Time (micro s)"
set xlabel "#N"
set title "Total Time, Calculation and  Comunication only for MPI"
set key right top

# Definir la paleta de colores
set style fill solid 1.0 border  -1

# Dibujar las barras
plot "tiempos.dat" using 2:xtic(1) title "Total time" lc rgb "red", \
     '' using 3 title "Calculation time" lc rgb "blue", \
     '' using 4 title "Comunication time" lc rgb "green"


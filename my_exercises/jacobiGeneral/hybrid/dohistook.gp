set terminal pngcairo
set output 'graficov2.png'

# Cargar los datos
set style data histogram
set style histogram clustered gap 1
set boxwidth 0.5
set grid y

set ylabel "Time (micro s)"
set xlabel "#N"
set title "Total Time, comp and comm for jacobi hybrid"
set key right top

# Definir la paleta de colores
set style fill solid 1.0 border -1

# Dibujar las barras
plot "tiempos.datg" using 2:xtic(1) title "Total time" lc rgb "red", \
     '' using 3 title "Comp time" lc rgb "blue", \
     '' using 4 title "Comm time" lc rgb "green"


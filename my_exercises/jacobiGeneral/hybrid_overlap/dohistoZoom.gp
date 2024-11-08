set terminal pngcairo
set output 'graficoZoom.png'

# Configuración de estilo para las barras
set style data histogram
set style histogram clustered gap 1
set boxwidth 0.5
set grid y

set ylabel "Time (micro s)"
set xlabel "#N"
set title "Total Time, comp and comm for jacobi hybrid overlap"
set key top right

# Definir el relleno y bordes de las barras
set style fill solid 1.0 border -1

# Dibujar las barras: seleccionando solo las filas desde N=8 hasta N=64
plot "tiempos.datg" every ::4::7 using 2:xtic(1) title "Total time" lc rgb "red", \
     '' every ::4::7 using 3 title "comp time" lc rgb "blue", \
     '' every ::4::7 using 4 title "comm time" lc rgb "green"


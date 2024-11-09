# Archivo: plot_barras.gp
set terminal pngcairo size 800,600 enhanced font 'Verdana,10'
set output 'FFTW_v1.png'

set title "Time vs number of process"
set xlabel " np "
set ylabel "Time (s)"
set style data histograms
set style fill solid border -1
set boxwidth 0.5
set grid ytics

plot 'timesite61.datg' using 2:xtic(1) title "time" lc rgb "blue"

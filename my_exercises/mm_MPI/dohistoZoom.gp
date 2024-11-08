set terminal pngcairo size 800,600
set output 'comparacion_tiempos_Zoom.png'
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9
set grid ytics
set xtics format 'N=%g' ##rotate by -45
set title 'mm naive vs dgemm'
set xlabel 'N'
set ylabel 'Time micro seconds'

plot 'tiemposZoom.datg' using 2:xtic(1) title 'Naive Total' linecolor rgb "red", \
     '' using 3 title 'Naive Comm' linecolor rgb "green", \
     '' using 4 title 'Naive Comp' linecolor rgb "blue", \
     '' using 5 title 'dgemm Total' linecolor rgb "orange", \
     '' using 6 title 'dgemm Comm' linecolor rgb "purple", \
     '' using 7 title 'dgemm Comp' linecolor rgb "cyan"


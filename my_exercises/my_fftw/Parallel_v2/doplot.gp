# Archivo: plot_barras.gp
set terminal pngcairo size 800,600 enhanced font 'Verdana,10'
#set terminal pngcairo size 800,600
set output 'FFTW_ori_vs_my.png'

set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9

set title "FFTW vs my_FFTW"
set xlabel "np"
set ylabel "Time (s)"
set xtics format "%.0f"
set grid ytics

set key outside top right

plot 'timesite61.datg' using 2:xtic(1) title 'FFTW' linecolor rgb "blue", \
     '' using 3 title 'MyFFTW' linecolor rgb "red"
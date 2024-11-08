set terminal pngcairo size 800,600 enhanced font 'Arial,12'
set output 'elapsed_times_comparison_by_iteration.png'

set style data histograms
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9

set title "Comparison of Elapsed Time per Iteration for Different np"
set xlabel "Iteration"
set ylabel "Elapsed time (s)"
set key outside

set grid ytics

plot 'elapsed_times.datg' using 2:xtic(1) title 'np=1', \
     '' using 3 title 'np=2', \
     '' using 4 title 'np=4', \
     '' using 5 title 'np=8', \
     '' using 6 title 'np=16', \
     '' using 7 title 'np=20'


# Set output file name
set output 'histo_results.png'

# Reset the plot settings
reset

# Set box width and grid style
set boxwidth 0.5
set grid ytics linestyle 0

# Set fill style for boxes
set style fill solid 0.20 border

# Set terminal type to PNG
set terminal png size 1200,800 font "Arial,10"

# Set plot title, x-axis label, and y-axis label
set title "Histogram of Jacobi size 100 using openmp, cpuinfo 2 threads, then 3 and 4 is oversuscribing"
set xlabel "threads"
set ylabel "time in micro seconds"

# Plot the histogram data
plot 'resultsleo.dat' using 1:2:xtic(1) with boxes lc rgb "#0045FF" title "time", \
     'resultsleo.dat' using 1:($2+0.25):2 with labels title ""

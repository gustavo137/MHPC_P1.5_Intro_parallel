# Set terminal type to PNG
set terminal png size 1200,800 font "Arial,10"

# Set output file name
set output 'Results_leo.png'

# Reset the plot settings
reset

# Set box width and grid style
set boxwidth 0.5
set grid ytics linestyle 0

# Set fill style for boxes
set style fill solid 0.20 border

# Set plot title, x-axis label, and y-axis label
set title "Histogram of Jacobi size 1000 with 100 iterations using OpenMP"
set xlabel "Threads"
set ylabel "Time in microseconds"

# Plot the data
plot 'results_leo2.txt' using 1:2 with linespoints title "Time"


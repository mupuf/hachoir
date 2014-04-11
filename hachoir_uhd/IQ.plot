set datafile separator ","
plot 'build/burst_0.csv' using 0:1 w l, '' using 0:2 w l
pause -1

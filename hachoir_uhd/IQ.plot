set datafile separator ","
plot 'build/burst_0.csv' using 0:2 w l, '' using 0:3 w l
pause -1

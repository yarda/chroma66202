# live.gnuplot: GNUPLOT command file to plot live data produced by chroma66202

# Producing the plot on the same machine on which chroma66202 is running:
# $ chroma66202 -cpt -r-1 -y1 | awk -F";" '{print $1,$2;system("")}' >/tmp/data
# $ gnuplot live.gnuplot

# Producing live plot on the remote host
# Listener ('system()' is used to flush buffers, i.e. line buffering mode):
# $ nc -l -p 8886  | awk -F";" '{print $1,$2;system("")}' >/tmp/data
# Sender:
# $ chroma66202 -cpt -r-1 -y1 | nc -w10 host 8886
# On listener:
# $ gnuplot live.gnuplot

set terminal wxt size 1300,600
set xdata time
set timefmt "%Y-%m-%d %H:%M:%S"
set format x "%H:%M"
set title "Power consumption over time measured with chroma66202 power meter"
set xlabel "Time"
set ylabel "Power in Watts"
set autoscale 
set yrange [0:]
plot "/tmp/data" using 1:4 with linespoints notitle
pause 1
reread

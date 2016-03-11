# live.gnuplot: GNUPLOT command file to plot live data produced by chroma66202

# Producing the plot on the same machine on which chroma66202 is running:
# ./chroma66202 -cpt -r-1 -y1 >/tmp/data
# gnuplot live.gnuplot

# Producing live plot on the remote host
# Sender:
# ./chroma66202 -cpt -r-1 -y1 | nc -w10 host 8886
# Listener:
# stdbuf -oL nc -l -p 8886  | stdbuf -oL awk -F";" '{print $1,$2}' >/tmp/data
# gnuplot live.gnuplot

set terminal wxt size 1300,600
set xdata time
set timefmt "%Y-%m-%d %H:%M:%S"
set format x "%H:%M"
set autoscale 
set yrange [0:]
plot "/tmp/data" using 1:4 with linespoints
pause 1
reread

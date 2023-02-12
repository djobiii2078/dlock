set terminal pdf 
set boxwidth 0.9 relative
set style data histograms
set style histogram cluster
set style fill solid 1.0 border lt -1
set output "prelim_lock_diff.pdf"
plot for [COL=2:4:2] 'prelim.dat' using COL

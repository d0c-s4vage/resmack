## set terminal qt noenhanced
## set rmargin 80
## set key at screen 1, graph 1
## set key autotitle columnhead
## stats 'corpus_tests_data.txt' using 0 nooutput
## set logscale x
## plot for [i=0:4] 'corpus_tests_data.txt' using 1:5 index i with points, for [i=(STATS_blocks-5):(STATS_blocks-1)] 'corpus_tests_data.txt' using 1:5 index i with points
## 
## fstr(N) = sprintf('f%d(x) = a%d*x**2 + b%d*x + c%d', N, N, N, N)
## fitstr(N) = sprintf('fit f%d(x) ''corpus_tests_data.txt'' using (log($1)):(log($5)) index %d via a%d,b%d,c%d', N, N, N, N, N)
## 
## set macros
## set fit quiet
## do for [n=0:4] {
##     eval(fstr(n))
##     eval(fitstr(n))
##     eval(sprintf('replot exp(f%d(log(x)))', n, n, n))
## }
## do for [n=(STATS_blocks-5):(STATS_blocks-1)] {
##     eval(fstr(n))
##     eval(fitstr(n))
##     eval(sprintf('replot exp(f%d(log(x)))', n, n, n))
## }


set terminal qt noenhanced
set rmargin 80
set key at screen 1, graph 1
set key autotitle columnhead
stats 'corpus_tests_data.txt' using 0 nooutput
set logscale x

set samples 10000 

# # --------------------------------------------------
# # PLOT TOP 5 + BOTTOM 5 + RAND + smooth bezier lines
# # --------------------------------------------------
# #
# # first dataset is always RAND, so do the first six items (want RAND as a
# # baseline)
# plot \
#   for [i=0:5] 'corpus_tests_data.txt' using 1:5 index i with points lc (i+1), \
#   for [i=0:5] 'corpus_tests_data.txt' using 1:5 index i smooth bezier lw 2 lc (i+1+5), \
#   for [i=(STATS_blocks-6):(STATS_blocks-1)] 'corpus_tests_data.txt' using 1:5 index i with points lc (STATS_blocks-i+9), \
#   for [i=(STATS_blocks-6):(STATS_blocks-1)] 'corpus_tests_data.txt' using 1:5:(0.1) index i smooth bezier lw 2 lc (STATS_blocks-i+14)

# # --------------------------------------------------
# # PLOT TOP 20 + RAND with smooth bezier lines
# # --------------------------------------------------
plot \
  for [i=0:20] 'corpus_tests_data.txt' using 1:5 index i with points lc (i+1), \
  for [i=0:20] 'corpus_tests_data.txt' using 1:5 index i smooth bezier lw 2 lc (i+1+5), \

#plot \
#    for [i=0:(STATS_blocks-1)] 'corpus_tests_data.txt' using 1:5 index i smooth bezier lw 2 lc i

#fstr(N) = sprintf('f%d(x) = a%d*x**2 + b%d*x + c%d', N, N, N, N)
#fitstr(N) = sprintf('fit f%d(x) ''corpus_tests_data.txt'' using (log($1)):(log($5)) index %d via a%d,b%d,c%d', N, N, N, N, N)

#set macros
#set fit quiet
#do for [n=0:4] {
#    eval(fstr(n))
#    eval(fitstr(n))
#    eval(sprintf('replot exp(f%d(log(x)))', n, n, n))
#}
#do for [n=(STATS_blocks-5):(STATS_blocks-1)] {
#    eval(fstr(n))
#    eval(fitstr(n))
#    eval(sprintf('replot exp(f%d(log(x)))', n, n, n))
#}


# fstr(N) = sprintf('f%d(x) = a%d*x**2 + b%d*x + c%d', N, N, N, N)
# fitstr(N) = sprintf('fit f%d(x) ''corpus_tests_data.txt'' using (log($1)):(log($5)) index %d via a%d,b%d,c%d', N, N, N, N, N)
# stats 'corpus_tests_data.txt' using 0 nooutput
# 
# do for [n=0:(STATS_blocks - 1)] {
#     set terminal qt n noenhanced
#     set rmargin 60
#     #set key at screen 1, graph 1
#     set key autotitle columnhead
#     set logscale x
#     plot 'corpus_tests_data.txt' using 1:5 index n with points lt n
# 
#     set macros
#     set fit quiet
# 
#     eval(fstr(n))
#     eval(fitstr(n))
#     eval(sprintf('replot exp(f%d(log(x)))', n, n))
#     #eval(sprintf('replot exp(f%d(log(x))) title the_title(%d)', n, n))
# }

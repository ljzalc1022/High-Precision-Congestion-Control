import math

import sys
fct_f = open("./experiment-results/hpcc/fat/25G_30L/0.1s/dynamic_u0.6_FP_90/fct.txt")
out_f = open("./experiment-results/hpcc/fat/25G_30L/0.1s/dynamic_u0.6_FP_90/fct_pure.txt", "w")

out_f.write("flow size(B)\tFCT(ns)\n")

for line in fct_f.readlines():
    data = line.split(' ')
    flow_size = float(data[4])
    fct = float(data[6])
    out_f.write(f"{flow_size} {fct}\n")

# close files
fct_f.close()
out_f.close()

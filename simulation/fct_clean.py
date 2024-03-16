import math

import sys
import os
if len(sys.argv) != 2:
    print(f"usage {sys.argv[0]} <dataFolder>")
    exit()
fct_f = open(os.sep.join([sys.argv[1], "fct.txt"]))
out_f = open(os.sep.join([sys.argv[1], "fct_pure.txt"]), "w")

out_f.write("flow size(B)\tFCT(ns)\n")

for line in fct_f.readlines():
    data = line.split(' ')
    flow_size = float(data[4])
    fct = float(data[6])
    out_f.write(f"{flow_size} {fct}\n")

# close files
fct_f.close()
out_f.close()

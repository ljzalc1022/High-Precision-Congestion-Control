import math

def Average(a):
    # comput average of array a
    sum = 0
    for x in a:
        sum += x
    return sum / len(a)

import sys
fct_f = open("./experiment-results/hpcc/fat/25G_30L/0.1s/dynamic_u0.45_fnv/fct.txt")
out_f = open("./experiment-results/hpcc/fat/25G_30L/0.1s/dynamic_u0.45_fnv/fct_ana.txt", "w")

N = 3
partition_name = ["100KB", "1MB", "10MB"]
partition_point = [100000, 1000000, 10000000]
partition_arrays = [[], [], [], [], []]

for line in fct_f.readlines():
    data = line.split(' ')
    flow_size = float(data[4])
    fct = float(data[6])
    for i in range(N + 1):
        if i == N:
            partition_arrays[N].append(fct)
            break
        if flow_size > partition_point[i]:
            continue
        partition_arrays[i].append(fct)
        break

out_f.write("size\t\tavg(us)\t\tmedian(us)\t\t95%(us)\t\t99%(us)\n")
for i in range(N + 1):
    # print class
    if(i == 0):
        out_f.write('<=' + partition_name[0])
    elif(i == N):
        out_f.write('>' + partition_name[N - 1])
    else:
        out_f.write(partition_name[i - 1] + "~" + partition_name[i])
    out_f.write('\t\t')

    partition_arrays[i].sort()
    l = len(partition_arrays[i])

    # print avg and median fct
    out_f.write(str(Average(partition_arrays[i])))
    out_f.write('\t\t')
    out_f.write(str(partition_arrays[i][math.ceil(l * 0.5) - 1]))
    out_f.write('\t\t')

    # print 95% and 99% fct
    out_f.write(str(partition_arrays[i][math.ceil(l * 0.95) - 1]))
    out_f.write("\t\t")
    out_f.write(str(partition_arrays[i][math.ceil(l * 0.99) - 1]))
    out_f.write("\n")

# close files
fct_f.close()
out_f.close()

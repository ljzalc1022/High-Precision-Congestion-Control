import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import math

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

color_1 = '#A5C496'
color_2 = '#E0F1F7'
color_3 = '#9392BE'
color_4 = '#F4CEB4'
color_5 = '#74B69F'
color_6 = '#9180AC'
color_7 = '#998DB7'

global_font_size = 27

font = {
    #'family': 'Times New Roman',
    'weight': 'normal',
    'size': global_font_size,
}


def plot_size_cdf(file_name):
    f = open(file_name)
    f.readline()

    size = np.array([])
    for line in f.readlines():
        data = line.split(' ')
        size = np.append(size, int(data[4]))

    # assert(len(size) == 17096)        

    sorted_size = np.sort(size)
    yvals = np.arange(len(sorted_size)) / float(len(sorted_size))

    fig, ax = plt.subplots(figsize=(6, 4))

    plt.plot(sorted_size, yvals, color = color_4, lw = 3, ms = 13, ls = '--')
    # plt.semilogx(sorted_size, yvals, color = color_5, lw = 3, ms = 13, ls = '--')

    indices1 = np.where(sorted_size < 100000)[0]
    split1 = np.max(indices1)
    indices2 = np.where(sorted_size < 5000000)[0]
    split2 = np.max(indices2)

    plt.plot(sorted_size[split1], yvals[split1], marker='*', markersize=10, color=color_6)
    plt.plot(sorted_size[split2], yvals[split2], marker='*', markersize=10, color=color_6)
    
    # index = [math.log10(0.01), math.log10(0.5)]
    xtick = [0, 5e6, 10e6, 15e6, 20e6, 25e6, 30e6]
    xtags = ["0", "5", "10", "15", "20", "25", "30"]
    plt.xticks(xtick, xtags, fontsize=global_font_size)

    ytick = [0.00, 0.25, 0.50, 0.75, 1.00]
    ytags = ["0", "", "0.5", "", "1.0"]
    plt.yticks(ytick, ytags, fontsize=global_font_size)
    plt.tick_params(labelsize=global_font_size)

    ax.set_xlabel("Flow Size (MB)", fontsize=global_font_size)
    ax.set_ylabel("CDF", fontsize=global_font_size)
    ax.set_axisbelow(True)

    fig.tight_layout()
    plt.subplots_adjust(left=0.20, right=0.98, bottom=0.22, top=0.94)

if __name__ == "__main__":
    plot_size_cdf("mix/fat/flow_distribution/WebSearch_25G_30%_0.1s.txt")
    
    plt.savefig("flow_distribution_load30.pdf")
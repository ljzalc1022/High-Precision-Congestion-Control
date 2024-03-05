import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import sys

prefix = ""
postfix = ""
setting = ""

postfix = "MAGIC"

if len(sys.argv) == 3:
    prefix += sys.argv[1]
    postfix = sys.argv[2]
else:
    exit()

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

data_dir = "./fct/"

# quantile_1 = 0.01
# quantile_2 = 0.30
quantile_1 = 0.55
quantile_2 = 0.80

if __name__ == "__main__":
    data = pd.read_csv("./experiment-results/hpcc/fat/25G_30L/0.1s/baseline/fct_pute.txt",
                       header=None,
                       names=["flow_size", "FCT"],
                       skiprows=[0],
                       delim_whitespace=True)
    baseline_sorted = data.sort_values(by=["flow_size", "FCT"])

    # fsSum = baseline_sorted.sum().iloc[0]
    # _33_quantile = fsSum * quantile_1
    # _66_quantile = fsSum * quantile_2
    _33_quantile = baseline_sorted.quantile(q=quantile_1).iloc[0]
    _66_quantile = baseline_sorted.quantile(q=quantile_2).iloc[0]
    # baseline_sorted["flow_size_sum"] = baseline_sorted.flow_size.cumsum()
    # baseline_sorted_33_ = baseline_sorted[baseline_sorted["flow_size_sum"] <= _33_quantile]
    # baseline_sorted_99_ = baseline_sorted[baseline_sorted["flow_size_sum"] > _66_quantile]
    # baseline_sorted_66_ = baseline_sorted[baseline_sorted["flow_size_sum"] <= _66_quantile]
    # baseline_sorted_66_ = baseline_sorted_66_[baseline_sorted_66_["flow_size_sum"] > _33_quantile]
    baseline_sorted_33_ = baseline_sorted[baseline_sorted["flow_size"] <= _33_quantile]
    baseline_sorted_99_ = baseline_sorted[baseline_sorted["flow_size"] > _66_quantile]
    baseline_sorted_66_ = baseline_sorted[baseline_sorted["flow_size"] <= _66_quantile]
    baseline_sorted_66_ = baseline_sorted_66_[baseline_sorted_66_["flow_size"] > _33_quantile]

    print(baseline_sorted_33_.tail(n=1))
    print()
    print(baseline_sorted_66_.tail(n=1))
    print()
    print(baseline_sorted_99_.tail(n=1))
    print()

    print(baseline_sorted_33_.mean().iloc[1],
        #  baseline_sorted_33_.median().iloc[1],
          baseline_sorted_33_.quantile(q=0.95).iloc[1],
          baseline_sorted_33_.quantile(q=0.99).iloc[1])
    print(baseline_sorted_66_.mean().iloc[1],
        #  baseline_sorted_66_.median().iloc[1],
          baseline_sorted_66_.quantile(q=0.95).iloc[1],
          baseline_sorted_66_.quantile(q=0.99).iloc[1])
    print(baseline_sorted_99_.mean().iloc[1],
        #  baseline_sorted_99_.median().iloc[1],
          baseline_sorted_99_.quantile(q=0.95).iloc[1],
          baseline_sorted_99_.quantile(q=0.99).iloc[1])
    # print(baseline_sorted)
    baseline_sorted_mean = [
        baseline_sorted_33_.mean().iloc[1],
        baseline_sorted_66_.mean().iloc[1],
        baseline_sorted_99_.mean().iloc[1]
    ]
    # baseline_sorted_median = [
    #     baseline_sorted_33_.median().iloc[1],
    #     baseline_sorted_66_.median().iloc[1],
    #     baseline_sorted_99_.median().iloc[1]
    # ]
    baseline_sorted_95_ = [
        baseline_sorted_33_.quantile(q=0.95).iloc[1],
        baseline_sorted_66_.quantile(q=0.95).iloc[1],
        baseline_sorted_99_.quantile(q=0.95).iloc[1]
    ]
    baseline_sorted_99_ = [
        baseline_sorted_33_.quantile(q=0.99).iloc[1],
        baseline_sorted_66_.quantile(q=0.99).iloc[1],
        baseline_sorted_99_.quantile(q=0.99).iloc[1]
    ]

    print('--------------------------------------')

    data = pd.read_csv(data_dir + prefix + "-dcqcn-fct-data-" + postfix + ".dat",
                       header=None,
                       names=["flow_size", "FCT"],
                       skiprows=[0],
                       delim_whitespace=True)
    magic_sorted = data.sort_values(by=["flow_size", "FCT"])
    # fsSum = magic_sorted.sum().iloc[0]
    # _33_quantile = fsSum * quantile_1
    # _66_quantile = fsSum * quantile_2
    _33_quantile = magic_sorted.quantile(q=quantile_1).iloc[0]
    _66_quantile = magic_sorted.quantile(q=quantile_2).iloc[0]
    # magic_sorted["flow_size_sum"] = magic_sorted.flow_size.cumsum()
    # magic_sorted_33_ = magic_sorted[magic_sorted["flow_size_sum"] <= _33_quantile]
    # magic_sorted_99_ = magic_sorted[magic_sorted["flow_size_sum"] > _66_quantile]
    # magic_sorted_66_ = magic_sorted[magic_sorted["flow_size_sum"] <= _66_quantile]
    # magic_sorted_66_ = magic_sorted_66_[magic_sorted_66_["flow_size_sum"] > _33_quantile]
    magic_sorted_33_ = magic_sorted[magic_sorted["flow_size"] <= _33_quantile]
    magic_sorted_99_ = magic_sorted[magic_sorted["flow_size"] > _66_quantile]
    magic_sorted_66_ = magic_sorted[magic_sorted["flow_size"] <= _66_quantile]
    magic_sorted_66_ = magic_sorted_66_[magic_sorted_66_["flow_size"] > _33_quantile]

    print(magic_sorted_33_.mean().iloc[1],
        #   magic_sorted_33_.median().iloc[1],
          magic_sorted_33_.quantile(q=0.95).iloc[1],
          magic_sorted_33_.quantile(q=0.99).iloc[1])
    print(magic_sorted_66_.mean().iloc[1],
        #   magic_sorted_66_.median().iloc[1],
          magic_sorted_66_.quantile(q=0.95).iloc[1],
          magic_sorted_66_.quantile(q=0.99).iloc[1])
    print(magic_sorted_99_.mean().iloc[1],
        #   magic_sorted_99_.median().iloc[1],
          magic_sorted_99_.quantile(q=0.95).iloc[1],
          magic_sorted_99_.quantile(q=0.99).iloc[1])
    # print(magic_sorted)
    magic_sorted_mean = [
        magic_sorted_33_.mean().iloc[1],
        magic_sorted_66_.mean().iloc[1],
        magic_sorted_99_.mean().iloc[1]
    ]
    # magic_sorted_median = [
    #     magic_sorted_33_.median().iloc[1],
    #     magic_sorted_66_.median().iloc[1],
    #     magic_sorted_99_.median().iloc[1]
    # ]
    magic_sorted_95_ = [
        magic_sorted_33_.quantile(q=0.95).iloc[1],
        magic_sorted_66_.quantile(q=0.95).iloc[1],
        magic_sorted_99_.quantile(q=0.95).iloc[1]
    ]
    magic_sorted_99_ = [
        magic_sorted_33_.quantile(q=0.99).iloc[1],
        magic_sorted_66_.quantile(q=0.99).iloc[1],
        magic_sorted_99_.quantile(q=0.99).iloc[1]
    ]

    print('--------------------------------------')

    mean_result = np.divide(magic_sorted_mean, baseline_sorted_mean)
    # median_result = np.divide(baseline_sorted_median, magic_sorted_median)
    _95_result = np.divide(magic_sorted_95_, baseline_sorted_95_)
    _99_result = np.divide(magic_sorted_99_, baseline_sorted_99_)

    result = np.array([mean_result, _95_result, _99_result])
    #result = np.array([mean_result, median_result, _95_result, _99_result])
    result = result.transpose()

    # _33_result = [magic_sorted_33_.mean().iloc[1] / baseline_sorted_33_.mean().iloc[1], magic_sorted_33_.median().iloc[1] / baseline_sorted_33_.median().iloc[1],
    #               magic_sorted_33_.quantile(q=0.95).iloc[1] / baseline_sorted_33_.quantile(q=0.95).iloc[1], magic_sorted_33_.quantile(q=0.99).iloc[1] / baseline_sorted_33_.quantile(q=0.99).iloc[1]]
    # _66_result = [magic_sorted_66_.mean().iloc[1] / baseline_sorted_66_.mean().iloc[1], magic_sorted_66_.median().iloc[1] / baseline_sorted_66_.median().iloc[1],
    #               magic_sorted_66_.quantile(q=0.95).iloc[1] / baseline_sorted_66_.quantile(q=0.95).iloc[1], magic_sorted_66_.quantile(q=0.99).iloc[1] / baseline_sorted_66_.quantile(q=0.99).iloc[1]]
    # _99_result = [magic_sorted_99_.mean().iloc[1] / baseline_sorted_99_.mean().iloc[1], magic_sorted_99_.median().iloc[1] / baseline_sorted_99_.median().iloc[1],
    #               magic_sorted_99_.quantile(q=0.95).iloc[1] / baseline_sorted_99_.quantile(q=0.95).iloc[1], magic_sorted_99_.quantile(q=0.99).iloc[1] / baseline_sorted_99_.quantile(q=0.99).iloc[1]]

    # print(_33_result)
    # print(_66_result)
    # print(_99_result)
    print("Mean ", mean_result)
    #print("Median ", median_result)
    print("95% ", _95_result)
    print("99% ", _99_result)

    color_1 = '#A5C496'
    color_2 = '#E0F1F7'
    color_3 = '#9392BE'
    color_4 = '#F4CEB4'
    color_5 = '#74B69F'
    color_6 = '#9180AC'
    color_7 = '#998DB7'

    fig, ax = plt.subplots(figsize=(6, 4))
    index = np.arange(3)

    bar_1 = plt.bar(index - 0.25, result[0], width=0.25, label='Short', color=color_4)
    bar_2 = plt.bar(index, result[1], width=0.25, label='Median', color=color_5)
    bar_3 = plt.bar(index + 0.25, result[2], width=0.25, label='Long', color=color_6)

    global_font_size = 27
    font = {
        # 'family': 'Times New Roman',
        'weight': 'normal',
        'size': global_font_size,
    }
    legend = plt.legend(handles=[bar_1, bar_2, bar_3], loc="right",
                        prop=font, frameon=False,
                        labelspacing=0.01, handlelength=0.7,
                        handletextpad=0.2, borderaxespad=0.0001,
                        columnspacing=0.5, ncol=6, bbox_to_anchor=(.83, 0.89, 0.2, 0.4))

    plt.ylim(0, 1.1)
    plt.yticks([0, 0.25, 0.5, 0.75, 1], ["0", "", "0.5", "", "1.0"])

    # plt.xlim()
    plt.xticks(index, ["Average", "95-pct", "99-pct"], fontsize=global_font_size)
    #plt.xticks(index, ["Average", "50-pct", "95-pct", "99-pct"])
    plt.tick_params(labelsize=global_font_size)

    plt.grid(True, axis="y", linestyle='--', alpha=0.5, linewidth=2)
    ax.set_ylabel("Relative FCT", fontsize=global_font_size)
    ax.set_axisbelow(True)

    fig.tight_layout()
    plt.subplots_adjust(left=0.2, bottom=0.14, right=0.99, top=0.87)
    plt.savefig("./DCQCN_relativeFCT_" + prefix + "-" + postfix + "-fct.pdf")

    # flow_size = magic_sorted["flow_size"]
    # hist, bins = np.histogram(flow_size, bins=10000, density=True)
    # cdf = np.cumsum(hist / sum(hist))
    # # print(*bins)
    # plt.plot(bins[1:], cdf)
    # plt.show()

    #
    # compareDF = pd.concat([baseline_sorted, magic_sorted["FCT"]], axis=1, join="inner")
    # compareDF.columns.values[3] = "magic_FCT"
    # compareDF.columns.values[1] = "baseline_FCT"
    # compareDF["cmp_1"] = np.where(compareDF["baseline_FCT"] > compareDF["magic_FCT"], 1, 0)
    # compareDF["cmp_0"] = np.where(compareDF["baseline_FCT"] > compareDF["magic_FCT"], 0, 1)
    #
    # compareDF["cmp_sum_1"] = compareDF.cmp_1.cumsum()
    # compareDF["cmp_sum_0"] = compareDF.cmp_0.cumsum()
    # print(compareDF)
    # plt.subplot()
    # plt.plot(np.arange(0, compareDF.shape[0]), compareDF["cmp_sum_1"])
    # plt.plot(np.arange(0, compareDF.shape[0]), compareDF["cmp_sum_0"])

    # plt.show()

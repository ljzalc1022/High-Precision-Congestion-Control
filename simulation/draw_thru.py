import sys
import matplotlib.pyplot as plt

if __name__ == "__main__":
    for i in range(5):
        file_name = "experiment-results/hpcc/conv/flow-thru-" + str(i) + ".dat"
        file = open(file_name)
        file.readline()

        x = []
        y = []
        for line in file.readlines():
            data = line.split(" ")
            if float(data[0]) < 0.065:
                x.append(float(data[0]))
                y.append(float(data[1]))
        
        plt.plot(x, y, label="flow-"+str(i))
    plt.xlabel("Time(s)")
    plt.ylabel("Throughput(Gbps)")
    plt.title("Throughput of all 5 flows(flow 4 is new flow, flow 0-3 are existing flow)")
    #   plt.title("Throughput flow " + str(i))
    plt.legend()
    #   plt.savefig("thru-flow-" + str(i))
    plt.savefig("thru-flow-magic")
    plt.clf()

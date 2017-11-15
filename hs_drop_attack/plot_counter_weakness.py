
import os, sys
import matplotlib
from matplotlib import pyplot as plt
import pdb
import argparse
import numpy as np

parser = argparse.ArgumentParser(description="")
parser.add_argument("--path", help="gives the root path of the chutney directory")
parser.add_argument("--nodedir", help="gives the name of the node directory where files are located")
parser.add_argument("--guard", help="Nickname of the HS guard")



if __name__ == "__main__":

    args = parser.parse_args()
    values = {}
    all_guard_measurements_x, all_guard_measurements_y = [], []
    for dirname in os.listdir(os.path.join(os.path.join(args.path, "net/"), args.nodedir)):
        if 'r' not in dirname:
            continue
        values[dirname] = [0, 0] #read, write
        with open(os.path.join(os.path.join(os.path.join(os.path.join(args.path, "net/"), args.nodedir), dirname), "state"), "r") as f:
            for line in f:
                if line.startswith("BWHistoryReadValues") or line.startswith("BWHistoryWriteValues"):
                    vals = line.split(' ')[1].split(',')
                    for val in vals:
                        if '\n' in val:
                            val = val[:-1]
                        if "Read" in line:
                            values[dirname][0] += int(val)
                            if dirname == args.guard:
                                #In this case, plot also all measurements
                                all_guard_measurements_x.append(int(val))
                        elif "Write" in line:
                            values[dirname][1] += int(val)
                            if dirname == args.guard:
                                #In this case, plot also all measurements
                                all_guard_measurements_y.append(int(val))

    fig, ax = plt.subplots()

    index = np.arange(len(values))
    bar_width = 0.35
    values_it = values.items()
    values_it.sort(key=lambda x: x[0])
    plt.bar(index, [x[1][0] for x in values_it], bar_width, label="Read")
    plt.bar(index+bar_width, [x[1][1] for x in values_it], bar_width, label="Write", color="r")
    plt.xticks(index + bar_width / 2, [x[0] for x in values_it], rotation=45)
    plt.ylabel("bytes")
    plt.xlabel("relays")
    plt.legend(loc="upper left")
    

    plt.tight_layout()
    plt.show()
    fig, ax = plt.subplots()
    pdb.set_trace()
    plt.plot(np.arange(len(all_guard_measurements_x))+1, all_guard_measurements_x, color="orange")
    plt.plot(np.arange(len(all_guard_measurements_y))+1, all_guard_measurements_y, color="black")
    plt.tight_layout()
    plt.show()





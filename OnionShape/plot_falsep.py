import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import argparse
import os, pdb
import datetime
import matplotlib.dates as md

parser = argparse.ArgumentParser(description='plot false positives from OnionShape')
subparsers = parser.add_subparsers(help='plot commands', dest='subparser')
plotfp_parser = subparsers.add_parser('plotfp')
nbrtry_parser = subparsers.add_parser('nbrtry')

plotfp_parser.add_argument("--in_dir")
plotfp_parser.add_argument("--file", action='append', help="file with data from the output of compute_avg_fpositive.py")
plotfp_parser.add_argument("--color", action='append', help="plot line color")
plotfp_parser.add_argument("--legend", action='append', help="plot line legend")
plotfp_parser.add_argument("--plt_att_win", action='store_true', help="plot fp y and attack window size in x")
plotfp_parser.add_argument("--thresh_variation", dest='thresh_variation', action='store_true')

nbrtry_parser.add_argument("--in_dir")
nbrtry_parser.add_argument("--startwith", nargs='*')
nbrtry_parser.add_argument("--thresholds", type=float, nargs='*')


def extract_values_nbrtry(pathnames):
    pathnames.sort()
    x = range(1, len(pathnames)+1)
    y = []
    i = 0
    relays = {}
    counter = 1
    for pathname in pathnames:
        with open(pathname) as f:
            for line in f:
                fp = line.split('.')[0].split(' ')[1]
                if fp not in relays:
                    relays[fp] = 1
                else:
                    relays[fp]+=1
            y_i = 0
            for elem in relays.values():
                if elem == counter:
                    y_i+=1
            y.append(y_i)
        counter+=1
    return x, y
#note to myself: choose easily parsable filename in the future --'
def extract_values(pathname, plt_att_win):
    x_y = {}
    with open(pathname, 'r') as f:
        for line in f:
            line_tab = line.split(' ')
            if plt_att_win:
                x_y[int(line_tab[0].split('.')[1].split('-')[0])] = float(line_tab[1][:-1])
            else:
                x_y[float("{0}.{1}".format(line_tab[0].split('.')[2],
                line_tab[0].split('.')[3].split('-')[0]))] = float(line_tab[1][:-1])
    x_y_list = x_y.items()
    x_y_list.sort(key=lambda x: x[0])
    return [x for x,y in x_y_list], [y for x,y in x_y_list]
def extract_values_thresh_variation(pathname):
    timestamps = []
    y = []
    with open(pathname, 'r') as f:
        for line in f:
            line_tab = line.split(':')
            timestamps.append(int(line_tab[0]))
            y.append(float(line_tab[1]))
    dates = [datetime.datetime.fromtimestamp(ts) for ts in timestamps]
    return dates, y
if __name__ == "__main__":
    args = parser.parse_args()
    if args.subparser == "plotfp":
        files = args.file
        colors = args.color
        legends = args.legend
        in_dir = args.in_dir
        assert len(files) == len(legends),\
        "not same numbers of input for file, color and legend"
        fix, ax = plt.subplots()
        if args.thresh_variation:
            #xfmt = md.DateFormatter('%Y-%m-%d')
            #ax.xaxis.set_major_formatter(xfmt)
            data = []
            for i in range(0, len(files)):
                x, y = extract_values_thresh_variation(os.path.join(in_dir, files[i]))
                #ax.plot(x, y, "-{0}".format(colors[i]), label=legends[i])
                data.append(y)
            ax.boxplot(data, labels=legends, showmeans=True)
            ax.legend(loc='upper right', shadow=True, fontsize=14)
            ax.set_ylabel('Threshold', fontsize=18)
            plt.xticks(rotation=25)
        else:
            max_y = 0
            for i in range(0, len(files)):
                x, y = extract_values(os.path.join(in_dir, files[i]), args.plt_att_win)
                max_y = max(max_y, y[0])
                ax.plot(x, y, "-{0}".format(colors[i]), label=legends[i])
            ax.legend(loc='upper right', shadow=True, fontsize=14)
            if args.plt_att_win:
                ax.set_xlabel("Attack window size", fontsize=18)
            else:
                ax.set_xlabel('Threshold', fontsize=18)
            ax.set_ylabel('#false positives', fontsize=18)
            ax.set_ylim(0, max_y+10)
        plt.grid()
        plt.show()
    elif args.subparser == "nbrtry":
        pathnames = {}
        fig, ax = plt.subplots()
        for threshold in args.thresholds:
            pathnames[threshold] = []
        for dirpath, dirnames, fnames in os.walk(args.in_dir):
            for fname in fnames:
                if "attack_started" in fname and "unordered" in fname:
                    for threshold in args.thresholds:
                        if str(threshold) in fname:
                            pathnames[threshold].append(os.path.join(args.in_dir, fname))

        for threshold in args.thresholds:
            x, y = extract_values_nbrtry(pathnames[threshold])
            plt.plot(x, y, label="Threshold used: {0}".format(threshold))
        ax.set_xlabel("Number of observations (days)", fontsize=18)
        ax.set_ylabel("#false positives", fontsize=18)
        ax.set_xlim([1, 5])
        plt.xticks(x[0:4])
        ax.legend(loc="upper right", shadow=True, fontsize=14)
        plt.grid()
        plt.tight_layout()
        plt.show()

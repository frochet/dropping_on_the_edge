import matplotlib
matplotlib.use('PDF')
#matplotlib.rcParams['agg.path.chunksize'] = 10000
import matplotlib.pyplot as plt
import numpy as np
import argparse
import os, pdb
import datetime
import matplotlib.dates as md
from stem import Flag
import onionshape
import pdb
import numpy

parser = argparse.ArgumentParser(description="plot the cumulative probability of the consumed bandwidth over the capacity of the guards. Also used to generate probability of false negative graphs")
parser.add_argument("--in_dir")
parser.add_argument("--start_year", type=int)
parser.add_argument("--start_month", type=int)
parser.add_argument("--start_day", type=int)
parser.add_argument("--end_year", type=int)
parser.add_argument("--end_month", type=int)
parser.add_argument("--end_day", type=int)
parser.add_argument("--consnumber", type=int)
parser.add_argument("--plotexittoo", dest="plotexittoo", action='store_true')
parser.add_argument("--plotmiddletoo", dest="plotmiddletoo", action='store_true')
parser.add_argument("--showapprox", dest="showapprox", action='store_true')
parser.add_argument("--considerhsbw", dest="considerhsbw", type=int, nargs = '*') #must be bytes !
parser.add_argument("--plotmiddle", dest="plotmiddle", action='store_true')
parser.add_argument("--intervalsize", dest="intervalsize", type=int, nargs='*')
######
##
#  This code uses the output of onionshape.py process command ! This part must be run first.
##
#  Needs a large refactoring. Things has been added one after one, without careful thinking making
#  a lot of code duplication
##
######


##### Plotting functions #####
# Code cf, getcdf borrowed from pathsim_plot.py written by Aaron Johnson (https://github.com/torps)
## helper - cumulative fraction for y axis
def cf(d): return numpy.arange(1.0,float(len(d))+1.0)/float(len(d))

## helper - return step-based CDF x and y values
## only show to the 99th percentile by default
def getcdf(data, shownpercentile=0.99):
    data.sort()
    frac = cf(data)
    x, y, lasty = [], [], 0.0
    for i in xrange(int(round(len(data)*shownpercentile))):
        x.append(data[i])
        y.append(lasty)
        x.append(data[i])
        y.append(frac[i])
        lasty = frac[i]
    return (x, y)

#We consider the observ bw to be consumebw+hsbw, that would be
#the consumed bandwidth of the relay if the attack was in progress.
#The goal is too check if we see it in the reported measurement

def extract_data_simulated_with_hsbw_consumption(ns_file, hsbw):
    network_state = onionshape.get_n_network_states([ns_file])
    data_guard, data_middle = [], []
    consweight_guard, consweight_middle = [], []
    for relay in network_state.cons_rel_stats:
        desc = network_state.descriptors[relay]
        rel_stat = network_state.cons_rel_stats[relay]
        if desc.observ_bw == 0:
            print "skip relay, observed bandwidth is wrong.. (0)"
            continue
        elif desc.observ_bw*1.05 < (float(desc.write_history[-1][1])/desc.write_history_interval): #5% error authorized. Arbitrary
            print "skip relay, observed bandwidth is lower than the consumed bandwidth: diff +{0}%".format(((float(desc.write_history[-1][1])/desc.write_history_interval)/desc.observ_bw)*100 -100.0)
            continue
        consum_bw = float(desc.write_history[-1][1])/desc.write_history_interval #bytes/s
        sim_bw = consum_bw + hsbw#hsbw
        if sim_bw > desc.observ_bw:
            sim_bw = desc.observ_bw
        if Flag.EXIT in rel_stat.flags: #captures exit and guard+exit
            continue
        elif Flag.GUARD in rel_stat.flags: #captures guards only
            data_guard.append((consum_bw/float(sim_bw))*100)
            consweight_guard.append(rel_stat.consweight[0]) #measured bandwidth as seen by bw authority
        else:
            data_middle.append((consum_bw/float(sim_bw))*100)
            consweight_middle.append(rel_stat.consweight[0])
    return data_guard, data_middle, consweight_guard, consweight_middle


def extract_data(ns_file, intervalsize):
    network_state = onionshape.get_n_network_states([ns_file])
    data_guard, data_exit, data_middle = [], [], []
    guard_adv_bw = []
    consweight_guard, consweight_middle = [], []
    for relay in network_state.cons_rel_stats:
        desc = network_state.descriptors[relay]
        rel_stat = network_state.cons_rel_stats[relay]
        if desc.observ_bw == 0:
            print "skip relay, observed bandwidth is wrong.. (0)"
            continue
        elif desc.observ_bw*1.1 < (float(desc.write_history[-1][1])/desc.write_history_interval): #10% error authorized. Arbitrary
            print "skip relay, observed bandwidth is lower than the consumed bandwidth: diff +{0}%".format(((float(desc.write_history[-1][1])/desc.write_history_interval)/desc.observ_bw)*100 -100.0)
            continue
        # We consider Wee=1, Wed=1
        if Flag.EXIT in rel_stat.flags: #captures exit and guard+exit
            data_exit.append(((float(desc.write_history[-1][1])/desc.write_history_interval)/desc.observ_bw)*100)
        elif Flag.GUARD in rel_stat.flags: #captures guards only
            if args.intervalsize and intervalsize < 7:
                data_guard.append(((
                    (sum([x[1] for x in desc.write_history[-intervalsize:]])/float(intervalsize))
                    /desc.write_history_interval)/desc.observ_bw)*100)
            else:
                data_guard.append(((float(desc.write_history[-1][1])/desc.write_history_interval)/desc.observ_bw)*100)
            guard_adv_bw.append(desc.observ_bw) # measured bw as seen locally by the relay
            consweight_guard.append(rel_stat.consweight[0]) #measured bandwidth as seen by bw authority
        else:
            data_middle.append(((float(desc.write_history[-1][1])/desc.write_history_interval)/desc.observ_bw)*100)
            consweight_middle.append(rel_stat.consweight[0]) #measured bandwidth as seen by bw authority
    return data_guard, data_exit, data_middle, guard_adv_bw, consweight_guard, consweight_middle

# data is the form (consbw/advbw; consweight)

def frac_false_neg(data, frac):
    #data.sort(key=lambda x: x[0], reverse=True) #sort by consbw/advbw reversed
    tot_consweight = 0
    for load, consweight in data:
        tot_consweight+=consweight
    cur_frac = 0.0
    i = 0
    cur_consweight = 0.0
    while float(i+1)/float(len(data)+1) < frac:
        cur_consweight += data[i][1]
        i+=1
    #return neg prob:
    return cur_consweight/float(tot_consweight)

def false_neg_anaylysis_different_intervals(args, pathnames):
    fig = plt.figure(1)
    ax = plt.gca()
    ax.set_xlabel("Threshold", fontsize=18)
    ax.set_ylabel("Probability of a false negative", fontsize=18)

    for intervalsize in args.intervalsize:
        data_guard, data_exit, data_middle, data_guard_adv_bw, data_consweight_g, data_consweight_m= [], [], [], [], [], []
        for pathname in pathnames:
            data_g, data_e, data_m, guard_adv_bw, consweight_guard, consweight_middle= extract_data(pathname, intervalsize)
            data_guard.extend(data_g)
            data_middle.extend(data_m)
            data_guard_adv_bw.extend(guard_adv_bw)
            data_consweight_g.extend(consweight_guard)
            data_consweight_m.extend(consweight_middle)
        x, y = getcdf(data_guard[:], shownpercentile=1.0)
        data_fneg = zip(data_guard, data_consweight_g)
        data_fneg.sort(key=lambda x: x[0], reverse=True)
        x2, y2, y2_approx = [],[],[]
        for i in range(1, len(x)):
            if x[i] > 0.0 and 100/x[i] >= 1.0 and 100.0/x[i] <= 2.0: #plot until threshold 2.0
                x2.append(100.0/x[i])
                y2.append(frac_false_neg(data_fneg, 1.0-y[i]))
                if args.showapprox:
                    y2_approx.append(1.0-y[i])
        x2.reverse()
        y2.reverse()
        plt.figure(1)
        plt.plot(x2, y2, label="{0} hours".format(intervalsize*4))
    plt.grid()
    plt.tight_layout()
    ax.legend(loc="upper left", shadow=True, fontsize=14)
    plt.savefig("{0}-{1}-{2}_to_{3}-{4}-{5}_consnumber_{6}_intervalsize_falseneg".format(args.start_year, args.start_month, args.start_day,
        args.end_year, args.end_month, args.end_day, args.consnumber))

def false_neg_analysis(args, pathnames):
    fig = plt.figure(1)
    ax = plt.gca()
    ax.set_xlabel("Threshold", fontsize=18)
    ax.set_ylabel("Probability of a false negative", fontsize=18)
    # guard first
    for hsbw in args.considerhsbw:
        data_guard, data_exit, data_middle, data_guard_adv_bw, data_consweight_g, data_consweight_m= [], [], [], [], [], []
        for pathname in pathnames:
            data_g,  data_m, consweight_g, consweight_m = extract_data_simulated_with_hsbw_consumption(pathname, hsbw)
            data_guard.extend(data_g)
            data_middle.extend(data_m)
            data_consweight_g.extend(consweight_g)
            data_consweight_m.extend(consweight_m)
        x, y =  [], []
        ## ploting false neg graphs
        if args.plotmiddle:
            x, y = getcdf(data_middle[:], shownpercentile=1.0)
            data_fneg = zip(data_middle, data_consweight_m)
        else:
            x, y = getcdf(data_guard[:], shownpercentile=1.0)
            data_fneg = zip(data_guard, data_consweight_g)
        data_fneg.sort(key=lambda x: x[0], reverse=True)
        x2, y2, y2_approx = [],[],[]
        for i in range(1, len(x)):
            if x[i] > 0.0 and 100/x[i] >= 1.0 and 100.0/x[i] <= 2.0: #plot until threshold 2.0
                x2.append(100.0/x[i])
                y2.append(frac_false_neg(data_fneg, 1.0-y[i]))
                if args.showapprox:
                    y2_approx.append(1.0-y[i])
        x2.reverse()
        y2.reverse()
        plt.figure(1)
        ax.set_ylim(top=1.0)
        plt.plot(x2, y2, label="bw={0}MB/s".format(hsbw/1000000.0))
        if args.showapprox:
            y2_approx.reverse()
            plt.plot(x2, y2_approx, label="bw={0}MB/s with approx".format(hsbw/1000000.0))
    plt.grid()
    plt.tight_layout()
    ax.legend(loc="upper left", shadow=True, fontsize=14)
    plt.savefig("{0}-{1}-{2}_to_{3}-{4}-{5}_consnumber_{6}_considerhsbw_falseneg".format(args.start_year, args.start_month, args.start_day,
        args.end_year, args.end_month, args.end_day, args.consnumber))


if __name__ == "__main__":
    args = parser.parse_args()
    pathnames = []
    day = args.start_day
    month = args.start_month
    prepend_consnumber = '0' if args.consnumber <= 9 else ''
    for year in range(args.start_year, args.end_year+1):
        while (year < args.end_year and month <= 12) or (month <= args.end_month):
            prepend_month = '0' if month <= 9 else ''
            while (year < args.end_year and month <= 12 and day <= 31) or\
                (day <= args.end_day):
                prepend_day = '0' if day <= 9 else ''
                pathnames.append(os.path.join(args.in_dir, "{0}-{1}{2}-{3}{4}-{5}{6}-00-00-network_state".format(
                    year, prepend_month, month, prepend_day, day, prepend_consnumber,
                    args.consnumber)))
                day+=1
            day=1
            month+=1
        month=1
    pathnames.sort()
    
    if args.considerhsbw:
        false_neg_analysis(args, pathnames)
    elif args.intervalsize:
        false_neg_anaylysis_different_intervals(args, pathnames)
    else:
        # Since a descriptor is renewed every ~18 hours, we look once a day
        data_guard, data_exit, data_middle, data_guard_adv_bw, data_consweight_g, data_consweight_m= [], [], [], [], [], []
        for pathname in pathnames:
            data_g, data_e, data_m, guard_adv_bw, consweight_guard, consweight_middle= extract_data(pathname, args)
            data_guard.extend(data_g)
            data_middle.extend(data_m)
            data_guard_adv_bw.extend(guard_adv_bw)
            data_consweight_g.extend(consweight_guard)
            data_consweight_m.extend(consweight_middle)
            if args.plotexittoo:
                data_exit.extend(data_e)

        x, y = getcdf(data_guard[:], shownpercentile=1.0)
        if args.plotexittoo:
            x2, y2 = getcdf(data_exit[:], shownpercentile=1.0)
        if args.plotmiddletoo:
            x3, y3 = getcdf(data_middle[:], shownpercentile=1.0)

        fig, ax = plt.subplots()
        ax.set_xlabel("Fraction of consumed/advertised bandwidth (%)", fontsize=18)
        ax.set_ylabel("Cumulative fraction", fontsize=18)
        plt.grid()
        plt.tight_layout()
        plt.plot(x,y, label="Utilization of guard relays")
        #plt.axvline(x=71.43, color='k', label="71.43%")
        if args.plotexittoo:
            plt.plot(x2,y2, label="Utilization of exit relays")
        if args.plotmiddletoo:
            plt.plot(x3,y3, label="Utilization of middle relays")
        ax.legend(loc="lower right", shadow=True, fontsize=14)
        plt.savefig("{0}-{1}-{2}_to_{3}-{4}-{5}_consnumber_{6}".format(args.start_year, args.start_month, args.start_day,
            args.end_year, args.end_month, args.end_day, args.consnumber))
        fig2, ax2 = plt.subplots()
        ax2.set_xlabel("Threshold", fontsize=18)
        ax2.set_ylabel("Probability of a false negative", fontsize=18)
        x4, y4, y4_approx = [], [], []
        data_fneg = zip(data_guard, data_consweight_g) #careful about previous sorting!
        data_fneg.sort(key=lambda x: x[0], reverse=True)
        for i in range(1, len(x)):
            if x[i] > 0.0 and 100/x[i] >= 1.0 and 100.0/x[i] <= 2.0: #plot until threshold 2.0
                x4.append(100.0/x[i])
                y4.append(frac_false_neg(data_fneg, 1.0-y[i]))
                if args.showapprox:
                    y4_approx.append(1.0-y[i])
        x4.reverse()
        y4.reverse()
        plt.grid()
        plt.tight_layout()
        plt.plot(x4, y4, label="guard-only relays")
        if args.showapprox:
            y4_approx.reverse()
            plt.plot(x4, y4_approx, label="approx for guard-only relays")
        if args.plotmiddletoo:
            x5, y5, y5_approx = [],[],[]
            data_fneg = zip(data_middle, data_consweight_m)
            data_fneg.sort(key=lambda x: x[0], reverse=True)
            for i in range(1, len(x3)):
                if x3[i] > 0.0 and 100.0/x3[i] >= 1.0 and 100.0/x3[i] <= 2.0:
                    x5.append(100.0/x3[i])
                    y5.append(frac_false_neg(data_fneg, 1.0-y3[i]))
                    if args.showapprox:
                        y5_approx.append(1.0-y3[i])
            x5.reverse()
            y5.reverse()
            plt.plot(x5,y5, label="unflagged relays")
            if args.showapprox:
                y5_approx.reverse()
                plt.plot(x5, y5_approx, label="approx for unflagged relays")
        ax2.legend(loc="upper left", shadow=True, fontsize=14)
        plt.savefig("{0}-{1}-{2}_to_{3}-{4}-{5}_consnumber_{6}_falsenegrisk".format(args.start_year, args.start_month, args.start_day,
            args.end_year, args.end_month, args.end_day, args.consnumber))
        ## Get correlation between the size of a relay and its consumption - Scatter plot?
        data_g = zip(data_guard_adv_bw, data_guard)
        data_g.sort(key=lambda x: x[0])
        fig3, ax3 = plt.subplots()
        ax3.set_xlabel("Bandwidth advertised (MB/s)", fontsize=18)
        ax3.set_ylabel("Fraction of consumed/advertised bandwidth (%)")
        plt.scatter([x[0]/100000.0 for x in data_g], [y[1] for y in data_g], s=1)
        plt.tight_layout()
        plt.savefig("scatterbw-{0}-{1}-{2}_to_{3}-{4}-{5}_consnumber".format(args.start_year, args.start_month, args.start_day,
            args.end_year, args.end_month, args.end_day, args.consnumber))





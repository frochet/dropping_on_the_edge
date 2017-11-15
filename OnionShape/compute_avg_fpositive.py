
import os, sys
from datetime import datetime
import pdb
import process_consensuses
import pytz
"""
To be executed after the sanitize_eval_fpositives_output.py - compute the average
number of positive relays during an attack window time, based 
"""

out_dir = "results/"

def main():
    pathnames = []
    for dirpath, dirnames, fnames in os.walk(out_dir):
        for fname in fnames:
            if "att_win" in fname and "unordered" in fname:
                attack_win = int(fname.split('.')[1].split('-')[0])
                fname_tab = fname.split('.')
                year_begin = int(fname_tab[4])
                month_begin = int(fname_tab[5])
                day_begin = int(fname_tab[6][:-1])
                year_end = int(fname_tab[7])
                month_end = int(fname_tab[8])
                day_end = int(fname_tab[9].split('-')[0])
                
                d1 = datetime(year_begin, month_begin, day_begin, tzinfo=pytz.UTC)
                d2 = datetime(year_end, month_end, day_end, tzinfo=pytz.UTC)

                time_elaspsed = process_consensuses.timestamp(d2) - process_consensuses.timestamp(d1)
                nbr_of_period = time_elaspsed/((attack_win+1)*4*60*60)
                nbr_lines = 0
                with open(os.path.join(out_dir, fname)) as f:
                    for line in f:
                        nbr_lines+=1
                print("{0} {1}").format(fname, float(nbr_lines)/float(nbr_of_period))

if __name__ == "__main__":
    main()

#!/bin/bash

#use this to make a statistical relevant number of search for relaydrop attack look alike
#within the history. Allows at the end to decide which value is the best pick for some
#parameters

BASE_DIR="CHANGEIT"
YEARBEGIN=2016
MONTHBEGIN=12
DAYBEGIN=2
YEAREND=2016
MONTHEND=12
DAYEND=31
PREV_WIN_SIZE=6
NEXT_WIN_SIZE=6
VARIANCE=$1
OVERHEAD_THRESHOLD_MAX=$(echo "2.5" | bc -l)
IN_DIR=$BASE_DIR/desc_dir/out/network-state-2016-12
OUT_DIR=$BASE_DIR/results
FILTERING="m"

mkdir -p $OUT_DIR

for ATTACKWIN in 1 2 3 6
do
  for OVERHEADTHRESHOLD in 1.0 1.05 1.1 1.15 1.2 1.25 1.3 1.35 1.4 1.45 1.5 1.55 1.6 1.65 1.7 1.75 1.8 1.85 1.9 1.95 2.0 2.05 2.1 2.15 2.2 2.25 2.3 2.35 2.4 2.45 2.5
  do
    if [ $VARIANCE == "1" ]
    then
      pypy onionshape.py relaydrop --start_year $YEARBEGIN --end_year $YEAREND --start_month $MONTHBEGIN --start_day $DAYBEGIN --end_month $MONTHEND --end_day $DAYEND --attack_win $ATTACKWIN --in_dir $IN_DIR --threshold $OVERHEADTHRESHOLD --filtering $FILTERING --prev_win_size $PREV_WIN_SIZE --next_win_size $NEXT_WIN_SIZE 1> $OUT_DIR/att_win.$ATTACKWIN-thresh.$OVERHEADTHRESHOLD-.$YEARBEGIN.$MONTHBEGIN.$DAYBEGIN-.$YEAREND.$MONTHEND.$DAYEND-filtering.$FILTERING-withvariance &
    else
      pypy onionshape.py relaydrop --start_year $YEARBEGIN --end_year $YEAREND --start_month $MONTHBEGIN --start_day $DAYBEGIN --end_month $MONTHEND --end_day $DAYEND --attack_win $ATTACKWIN --in_dir $IN_DIR --threshold $OVERHEADTHRESHOLD --filtering $FILTERING --prev_win_size $PREV_WIN_SIZE --next_win_size $NEXT_WIN_SIZE --no-variance 1> $OUT_DIR/att_win.$ATTACKWIN-thresh.$OVERHEADTHRESHOLD-.$YEARBEGIN.$MONTHBEGIN.$DAYBEGIN-.$YEAREND.$MONTHEND.$DAYEND-filtering.$FILTERING &
    fi
  done
done


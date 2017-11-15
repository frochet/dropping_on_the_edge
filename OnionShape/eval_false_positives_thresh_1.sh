#!/bin/bash

#use this to make a statistical relevant number of search for relaydrop attack look alike
#within the history. Allows at the end to decide which value is the best pick for some
#parameters

BASE_DIR="CHANGEIT"
YEARBEGIN=2016
MONTHBEGIN=12
DAYBEGIN=2
YEAREND=2017
MONTHEND=1
DAYEND=31
PREV_WIN_SIZE=12
NEXT_WIN_SIZE=12
VARIANCE=1
OVERHEADTHRESHOLD=$(echo "1.0" | bc -l)
IN_DIR=$BASE_DIR/desc_dir/out/ns_2016-12_2017-01
OUT_DIR=$BASE_DIR/results
FILTERING="g"

mkdir -p $OUT_DIR

END=24

for ATTACKWIN in $(seq 2 $END);
do
  if [ $VARIANCE == "1" ]
  then
    pypy onionshape.py relaydrop --start_year $YEARBEGIN --end_year $YEAREND --start_month $MONTHBEGIN --start_day $DAYBEGIN --end_month $MONTHEND --end_day $DAYEND --attack_win $ATTACKWIN --in_dir $IN_DIR --threshold $OVERHEADTHRESHOLD --filtering $FILTERING --prev_win_size $PREV_WIN_SIZE --next_win_size $NEXT_WIN_SIZE 1> $OUT_DIR/att_win.$ATTACKWIN-thresh.$OVERHEADTHRESHOLD-.$YEARBEGIN.$MONTHBEGIN.$DAYBEGIN-.$YEAREND.$MONTHEND.$DAYEND-filtering.$FILTERING-withvariance &
  else
    pypy onionshape.py relaydrop --start_year $YEARBEGIN --end_year $YEAREND --start_month $MONTHBEGIN --start_day $DAYBEGIN --end_month $MONTHEND --end_day $DAYEND --attack_win $ATTACKWIN --in_dir $IN_DIR --threshold $OVERHEADTHRESHOLD --filtering $FILTERING --prev_win_size $PREV_WIN_SIZE --next_win_size $NEXT_WIN_SIZE --no-variance 1> $OUT_DIR/att_win.$ATTACKWIN-thresh.$OVERHEADTHRESHOLD-.$YEARBEGIN.$MONTHBEGIN.$DAYBEGIN-.$YEAREND.$MONTHEND.$DAYEND-filtering.$FILTERING &
  fi
done


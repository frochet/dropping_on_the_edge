# Traffic shaping detection tool 

Currently detect two kind of situations : 

- We know that it exists an onion address with lot of load. We know that
  this onion serive got down a couple of time (can be seen in the
hidden-service traffic graph as a drop of hidden-service traffic). 
  Then, we look for guard nodes (or any nodes, the operator might have
used the option EntryNodes) that are overloaded and then loose
utilization during the down of the HS. We match those nodes being the
entry nodes of the onion service

- We extract guards matching a overload signature of our relay_drop
  attack. looking, for example, at 3 following peaks of utilization. This
feature is used in our paper "Dropping on the Edge: Flexibility and
Traffic Confirmation in Onion Routing Protocols"

# Commands

First step is to use the process command which will parse the
descriptors and consensuses in the timing interval you are requesting  
  
For example:  
  
```bash
pypy onionshape.py process --start_year 2016 --end_year 2017
--start_month 12 --end_month 1 --in_dir_cons desc_dir/consdir
--in_dir_desc desc_dir/serverdir --in_dir_extra_desc desc_dir/extradir
--out_dir desc_dir/out
``` 

If the descriptors are not inside the in dirs, OnionShape downloads
them from CollectTor and stores them according to the directories you
specified. Though, you need to untar the downloaded file yourself before
launching the command again.  
Once the above command terminated, you should have stored an average of
5GB of pickle files per month in the out_dir. You may now search for
relay drop attack signature in the history. For example:

```bash
 pypy onionshape.py relaydrop --start_year 2016 --end_year 2016
--start_month 12 --start_day 17 --end_month 12 --end_day 21 --attack_win
1 --in_dir desc_dir/out/network-state-2016-12 --threshold 1.5
--filtering 'g' --attack_started_at '2016121903'  
```
To reproduce the results from our paper, run the .sh scripts after
changing the BASE_DIR variable inside the script. Then, run
sanitize_eval_fpositives_output.py. It should touch all files in the
directory results/  

(TODO: do it in the .sh script)

Finally, to plot the results:  

```bash
python plot_falsep.py --in_dir results --file mean_g_att_win_1 --color
"k" --legend "attack window 1 and mean only" --file mean_g_att_win_2
--color "r" --legend "attack window 2 and mean only" --file
mean_g_att_win_3 --color "b" --legend "attack window 3 and mean only"
--file mean_variance_att_win_2 --color ".r" --legend "attack window 2
and mean+variance" --file mean_variance_att_win_3 --color ".b" --legend
"attack window 3 and mean+variance"
```

Alternatively, we also added the possibility to look at the variation of
the threshold through time. For example, to compute the threshold
variation to get at most 5 false positives during December 2016:

```bash
pypy onionshape.py thresh_variation --start_year 2016 --end_year 2016
--start_month 12 --start_day 1 --end_month 12 --end_day 31 --attack_win
2 --in_dir desc_dir/out/network-state-2016-12 --objective 5  --filtering
'g' > thresh_variation_december2016_obj5_attwin_2
```
Then, to plot the results:  

```bash
python plot_falsep.py --in_dir results/ --file
thresh_variation_december2016_obj5_attwin_2 --color 'b' --legend
'attack_win 2' --thresh_variation
```

# Requirements

## Resources
- lot of rams (60GB to process 2 months)  

## Python package needed :  

- Stem  
- wget  
- pytz

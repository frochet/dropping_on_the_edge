import sys, os
import argparse
import process_consensuses
import pickle
import pdb
from datetime import datetime
from stem import Flag
import pytz

parser = argparse.ArgumentParser(description='OnionShape uses the bandwidth history to perform guard discovery attacks')

subparsers = parser.add_subparsers(help='OnionShape commands', dest='subparser')
process_parser = subparsers.add_parser('process',
        help='Process consensuses, descriptors and extra info descriptors to store \
              lightweight class from onionshape.py on a hourly basis between \
              the specified date')
process_parser.add_argument('--start_year', type=int)
process_parser.add_argument('--start_month', type=int)
process_parser.add_argument('--end_year', type=int)
process_parser.add_argument('--end_month', type=int)
process_parser.add_argument('--in_dir_cons', help='directory where are located consensus')
process_parser.add_argument('--in_dir_desc', help='directory where are located descriptors')
process_parser.add_argument('--in_dir_extra_desc', help='directory where are located extra escriptors')
process_parser.add_argument('--out_dir', help='directory where we output our lightweight classes')
process_parser.add_argument('--initial_descriptor_dir', help='Needed if we look in the beginning of a month, due to the structure of \
        archives files, the descriptors might be in the previous month')
process_parser.add_argument('--initial_extrainfo_descriptor_dir', help='Need if we look in the beginning of a month, due the structure of \
        archives files, the descriptors might be in the previous month')


pastOnions_parser = subparsers.add_parser('pastOnions',
        help='search inside the history for change of load on some guards matching \
              a downtime of a large hidden service - we should have the knowledge of such \
              downtime')

pastOnions_parser.add_argument('--threshold_prev', type=int, default=50, help='If the current window is < than average*threshold/100, we have a positive match')
pastOnions_parser.add_argument('--threshold_next', type=int, default=50, help='If the current window is < than average*threshold/100, we have a positive match')
pastOnions_parser.add_argument('--prev_win_size', type=int, default=6, help='Number of reported measurements of the previous window')
pastOnions_parser.add_argument('--cur_win_size', type=int, default=1, help='Number of reported measurements of the current window')
pastOnions_parser.add_argument('--next_win_size', type=int, default=6, help='Number of reported measurements of the next window')
pastOnions_parser.add_argument('--start_year', type=int)
pastOnions_parser.add_argument('--end_year', type=int)
pastOnions_parser.add_argument('--start_month', type=int)
pastOnions_parser.add_argument('--end_month', type=int)
pastOnions_parser.add_argument('--start_day', type=int)
pastOnions_parser.add_argument('--end_day', type=int)
pastOnions_parser.add_argument('--in_dir', help='directory where are stored pickle files')
pastOnions_parser.add_argument('--filtering', help='filter in flag request (can be e,g,d,m or a combinaison (e.g. g,d chooses all guard-flagged nodes)',
        default='g')

relaydrop_parser = subparsers.add_parser('relaydrop',
        help='search inside the history for our relay drop guard discoverry attack')
relaydrop_parser.add_argument('--start_day', type=int)
relaydrop_parser.add_argument('--end_day', type=int)
relaydrop_parser.add_argument('--start_year', type=int)
relaydrop_parser.add_argument('--end_year', type=int)
relaydrop_parser.add_argument('--start_month', type=int)
relaydrop_parser.add_argument('--end_month', type=int)
relaydrop_parser.add_argument('--in_dir', help='directory where are stored pickle files')
relaydrop_parser.add_argument('--prev_win_size', type=int, default=12, help='Number of reported measurements of the previous window')
relaydrop_parser.add_argument('--next_win_size', type=int, default=1, help='Number of reported measurements of the next window')
relaydrop_parser.add_argument('--attack_win' , help = 'Number of reported measurements while \
        the attack is applied', type=int, default=1)
relaydrop_parser.add_argument('--threshold', type=float, default=1.5,  help='mult coefficient of average observed bw to be positive')
relaydrop_parser.add_argument('--variance', dest='variance', action='store_true')
relaydrop_parser.add_argument('--no-variance', dest='variance', action='store_false')
relaydrop_parser.set_defaults(variance=True)
relaydrop_parser.add_argument('--attack_started_at', default=None, help='the time %Y%m%d%H when the relay drop attack has been launched on the network.\
if given, we output only positive results in the range [attack_start_at, attack_started_at+attack_win')
relaydrop_parser.add_argument('--filtering', help='filter in flag request (can be e,g,d,m or a combinaison (e.g. g,d chooses all guard-flagged nodes)',
        default='g')

thresh_variation_parser = subparsers.add_parser('thresh_variation',
        help='Looks for the variation of threshold through time for a maximum of N false positives, at any time, with a dichotomic search')

thresh_variation_parser.add_argument('--start_day', type=int)
thresh_variation_parser.add_argument('--end_day', type=int)
thresh_variation_parser.add_argument('--start_year', type=int)
thresh_variation_parser.add_argument('--end_year', type=int)
thresh_variation_parser.add_argument('--start_month', type=int)
thresh_variation_parser.add_argument('--end_month', type=int)
thresh_variation_parser.add_argument('--in_dir', help='directory where are stored pickle files')
thresh_variation_parser.add_argument('--prev_win_size', type=int, default=6, help='Number of reported measurements of the previous window')
thresh_variation_parser.add_argument('--next_win_size', type=int, default=6, help='Number of reported measurements of the next window')
thresh_variation_parser.add_argument('--attack_win' , help = 'Number of reported measurements while \
        the attack is applied', type=int, default=1)
thresh_variation_parser.add_argument('--variance', dest='variance', action='store_true')
thresh_variation_parser.add_argument('--no-variance', dest='variance', action='store_false')
thresh_variation_parser.set_defaults(variance=True)
thresh_variation_parser.add_argument('--starting_lower_bound', type=float, default=1.0)
thresh_variation_parser.add_argument('--starting_upper_bound', type=float, default=3.0)
thresh_variation_parser.add_argument('--objective', type=int, default=5)

thresh_variation_parser.add_argument('--filtering', help='filter in flag request (can be e,g,d,m or a combinaison (e.g. g,d chooses all guard-flagged nodes)',
        default='g')

DEFAULT_NUMBER_MEASUREMENTS = 6
DEFAULT_MEASUREMENT_INTERVAL = 4*60*60 #4 hours
MAX_HOLE_PERMITTED = 2 * DEFAULT_MEASUREMENT_INTERVAL #arbitrary
ONE_DAY = 24*60*60
_TESTING = False#a bit ugly to log for debugging purpose
_PRINT_SUSPECT = False
MIN_CONSWEIGHT_CONSIDERED = 1000

class RouterStatusEntry:
  
  count = 0
  def __init__(self, fprint, nickname, flags, consweight):
    self.consweight = [consweight]
    self.fprint = fprint
    self.flags = flags
    self.nickname = nickname
    self.prev_win = None
    self.cur_win = None
    self.next_win = None
    self.slice_value = None
    self.updated = False
  
  def update(self, router):
    self.flags = router.flags
    #Backward compatibility
    if type(self.consweight) != type([]):
      self.consweight = [self.consweight]
    self.consweight.append(router.consweight)
    self.nickname = router.nickname
  def has_new_values(self):
    return self.updated
  def are_windows_full(self):
    return self.prev_win.is_full() and self.cur_win.is_full() and self.next_win.is_full()
  
  def flush_windows(self):
    self.next_win.remove_all()
    self.cur_win.remove_all()
    self.prev_win.remove_all()

  def _check_init_windows(self, descriptor, prev_win_size, cur_win_size, next_win_size):
    #slice_value should be one for retour up-to-date (using h
    if self.slice_value is None:
      self.slice_value = ONE_DAY/(DEFAULT_NUMBER_MEASUREMENTS*descriptor.write_history_interval)
    else:
      tmp_slice_value = ONE_DAY/(DEFAULT_NUMBER_MEASUREMENTS*descriptor.write_history_interval)
      if tmp_slice_value != self.slice_value: #discard the windows in case the route has updated
        self.prev_win, self.cur_win, self.next_win = None, None, None
        self.slice_value = tmp_slice_value
        if _TESTING:
          print("{0}.{1} has changed the history interval measurements. Discarding windows and building new ones".
                format(self.fprint, self.nickname))
    if self.prev_win is None:
      self.prev_win = Window(prev_win_size*self.slice_value)
    if self.cur_win is None:
      self.cur_win = Window(cur_win_size*self.slice_value)
    if self.next_win is None:
      self.next_win = Window(next_win_size*self.slice_value)

  def _check_if_new_values(self, descriptor):
    def _maybe_flush_windows(timestamp, window):
      (ts, value) = window.data[-1]
      if timestamp - ts > MAX_HOLE_PERMITTED:
        if _TESTING:
          print("{0}.{1} flushes windows due to a hole of {2} seconds from measurement".
                format(self.fprint, self.nickname, timestamp-ts))
        self.flush_windows()

    if descriptor.write_history is None:
      if _TESTING:
        raise ValueError("Write_history should not be none")
      return False
    if self.prev_win.is_empty(): 
      return True
    for (timestamp, value) in descriptor.write_history:
      if self.next_win.is_newer(timestamp):
        _maybe_flush_windows(timestamp, self.next_win)
        return True
      elif self.cur_win.is_newer(timestamp) and self.next_win.is_empty():
        _maybe_flush_windows(timestamp, self.cur_win)
        return True
      elif self.prev_win.is_newer(timestamp) and self.cur_win.is_empty() and \
            self.next_win.is_empty():
        _maybe_flush_windows(timestamp, self.prev_win)
        return True
      else:
        continue
    return False
  
  def _slide_windows(self, descriptor):
    #slide slice_value measurement through windows
    for i in range(0, self.slice_value):
      self.prev_win.pop()
      self.prev_win.add(self.cur_win.pop())
      self.cur_win.add(self.next_win.pop())
      self.next_win.add_data(descriptor, self.cur_win)

  def update_windows(self, descriptor, prev_win_size, cur_win_size, next_win_size):
    RouterStatusEntry.count+=1
    #Firstly verify that this entry is not new
    self._check_init_windows(descriptor, prev_win_size, cur_win_size, next_win_size)
    if _TESTING:
      if RouterStatusEntry.count % 1000 == 0:
        print "prev_win: {0}, cur_win: {1}, next_win: {2}".format(self.prev_win.data,
                self.cur_win.data, self.next_win.data)
    if self._check_if_new_values(descriptor):
      self.updated = True
      #If we have not see enough consensuses, windows are empty and we fill them
      #through time
      if self.are_windows_full():
        self._slide_windows(descriptor)
      else:
          # add_data return the number of added measurement. These fill the windows
        r = self.prev_win.add_data(descriptor, None)
        if self._check_if_new_values(descriptor):
          self.cur_win.add_data(descriptor, self.prev_win)
          if self._check_if_new_values(descriptor):
            self.next_win.add_data(descriptor, self.cur_win)
    else:
      self.updated = False
      #if _TESTING:
        #print("Router {0}.{1} does not have new values".format(
            #self.fprint, self.nickname))

class ServerDescriptor:
    
    def __init__(self, fprint, hibernating, nickname, family, address,
            avg_bw, burst_bw, observ_bw, extra_info_digest, end_interval,
            write_history_interval, write_history):
        self.fprint = fprint
        self.hibernating = hibernating
        self.nickname = nickname
        self.address = address
        self.avg_bw = avg_bw
        self.burst_bw = burst_bw
        self.observ_bw = observ_bw
        self.extra_info_digest = extra_info_digest
        self.end_interval = end_interval
        self.write_history_interval = write_history_interval
        self.write_history = write_history # dict of {timestamp: measurement}

class NetworkStatusDocument:

    def __init__(self, cons_valid_after, cons_fresh_until, cons_bw_weights,
            cons_bwweightscale, relays):
        self.cons_bwweightscale = cons_bwweightscale
        self.cons_valid_after = cons_valid_after
        self.cons_fresh_until = cons_fresh_until
        self.cons_bw_weights = cons_bw_weights
        self.relays = relays

class NetworkState:
    def __init__(self, cons_valid_after, cons_fresh_until, cons_bw_weights,
            hibernating_statuses, cons_rel_stats, descriptors):
        self.cons_valid_after = cons_valid_after
        self.cons_fresh_until = cons_fresh_until
        self.cons_bw_weights = cons_bw_weights
        self.cons_rel_stats = cons_rel_stats
        self.descriptors = descriptors
        self.hibernating_statuses = hibernating_statuses 
    

class History:
    
    def __init__(self, prev_win_size, cur_win_size, next_win_size,
            descriptors, relays, ns_files, filtering):
        self.prev_win_size = prev_win_size
        self.cur_win_size = cur_win_size
        self.next_win_size = next_win_size
        self.descriptors = descriptors
        self.relays = relays
        self.filtering = filtering
        self.init_windows(ns_files)

    def init_windows(self, ns_files):
        #initializing windows
        if self.prev_win_size+self.cur_win_size+self.next_win_size > (len(ns_files)/24*6):
            raise ValueError("Windows have been choosen too large")
        network_state = get_n_network_states([ns_files[0]])
        ns_files = ns_files[1:]
        self.cur_time = network_state.cons_valid_after
        self.update_history(network_state)
    
    def _up_relays(self, cons_rel_stats):
        """
            add to self.relays every new relays and updates feature
            of existing relays
        """
        for fprint in cons_rel_stats:
          if fprint not in self.relays:
            self.relays[fprint] = cons_rel_stats[fprint]
          else:
            self.relays[fprint].update(cons_rel_stats[fprint])

    def update_history(self, network_state):
      self.cur_time = network_state.cons_valid_after
      if _TESTING or _PRINT_SUSPECT:
        print("Updating history at cons time {0}".format(network_state.cons_valid_after))
      if filtering == None:
        self._up_relays(network_state.cons_rel_stats)
        self.descriptors.update(network_state.descriptors)
      else:
        tmp_cons_rel_stats = {}
        tmp_descriptors = {}
        for fp in network_state.cons_rel_stats:
          relay = network_state.cons_rel_stats[fp]
          for flag in self.filtering:
            if flag == 'e':
              if Flag.EXIT in relay.flags and Flag.GUARD not in relay.flags:
                tmp_cons_rel_stats[fp] = relay
                tmp_descriptors[fp] = network_state.descriptors[fp]
            elif flag == 'g':
              if Flag.GUARD in relay.flags and Flag.EXIT not in relay.flags:
                tmp_cons_rel_stats[fp] = relay
                tmp_descriptors[fp] = network_state.descriptors[fp]
            elif flag == 'd':
              if Flag.EXIT and Flag.GUARD in relay.flags:
                tmp_cons_rel_stats[fp] = relay
                tmp_descriptors[fp] = network_state.descriptors[fp]
            else: #flag is 'm'
              if Flag.EXIT not in relay.flags and Flag.GUARD not in relay.flags:
                tmp_cons_rel_stats[fp] = relay
                tmp_descriptors[fp] = network_state.descriptors[fp]
        self._up_relays(tmp_cons_rel_stats)
        self.descriptors.update(tmp_descriptors)
        if _TESTING:
          print("Remaining {0} relays with flags {1} after filtering".format(len(tmp_cons_rel_stats),
              self.filtering))
      for fp in self.relays:
        relay = self.relays[fp]
        relay.update_windows(self.descriptors[fp], self.prev_win_size,
            self.cur_win_size, self.next_win_size)

class Window:
  def __init__(self, size):
      self.size = size
      self.is_updated = False
      self.data = []
      self.cur_size = 0
  
  def remove_all(self):
      self.cur_size = 0
      self.data = []

  def add(self, data_elem):
      """
        data_elem should be the form of (timestamp, value)
      """
      if self.is_full():
        raise ValueError("We add data in a full window, it should not happen")
      self.data.append(data_elem)
      self.cur_size += 1
    
  def is_newer(self, timestamp):
      if self.is_empty():
        return False
      (ts, data) = self.data[-1]
      return timestamp > ts
  def pop(self):
      if self.is_empty():
        raise ValueError("We pop a data from an empty window, it should not happen")
      self.cur_size -= 1
      return self.data.pop(0)

  def is_empty(self):
      return len(self.data) == 0

  def is_full(self):
      return len(self.data) == self.size
  def get_data(self):
      return self.data
  def get_win_values(self):
      r = []
      for (ts, value) in self.data:
        r.append(value)
      return r
  def add_data(self, descriptor, previous_window):
      """
       the previous window correspond to the window before self.
       if self is next_win, previous should be cur_win.
       if self is cur_win, previous should be prev_win
       if self is prev_win, previous should be None
       We make a safe check if the window is empty by verifying
       if the data we add is not in the previous window
      """
      added_data = 0
      for (timestamp, value) in descriptor.write_history:
        if previous_window == None: #self is prev_win
          if not self.is_empty():
            (ts2, value2) = self.data[-1] # should use a get_last_elem() to stay generic wrt data
          else:
            ts2 = 0
          if timestamp > ts2 and not self.is_full():
            self.add((timestamp, value))
            added_data += 1
        else:
          #match some edges cases where the previous_window is just filled with our data
          (ts2, value2) = previous_window.data[-1]
          if not self.is_empty():
            (ts3, value3) = self.data[-1]
          else:
            ts3 = 0
          if timestamp > ts2 and timestamp > ts3 and not self.is_full():
            self.add((timestamp, value))
            added_data += 1
      return added_data

def lookup_pastOnions(ns_files, prev_win_size, cur_win_size,
        next_win_size, threshold_prev, threshold_next, filtering):
    #We initiate the window and we do a first check
  descriptors = {}
  relays = {}
  history = History(prev_win_size, cur_win_size,
        next_win_size, descriptors, relays, ns_files, filtering)
  apply_onionshape_test(history, threshold_prev, threshold_next)
    #We move the windows on a hourly basis and
    #update windows if there is a new measurement for
    #their relays
  for ns_file in ns_files:
    network_state = get_n_network_states([ns_file])
    history.update_history(network_state)
    apply_onionshape_test(history, threshold_prev, threshold_next)

def find_relaydrop_attacked_relay(ns_files, prev_win_size, attack_win,
        next_win_size, threshold, attack_started_at, filtering, variance):
    descriptors = {}
    relays = {}
    ignore_next_win = False
    if next_win_size == -1: #we ignore the next win size in the relaydrop test
      next_win_size = 1 #smallest window the system accepts
      ignore_next_win = True

    history = History(prev_win_size, attack_win,
        next_win_size, descriptors, relays, ns_files, filtering)
    apply_relaydrop_test(history, threshold, attack_started_at, ignore_next_win, variance)
    for ns_file in ns_files:
      network_state = get_n_network_states([ns_file])
      history.update_history(network_state)
      apply_relaydrop_test(history, threshold, attack_started_at, ignore_next_win, variance)

def search_for_objective(history, lower_bound, upper_bound, objective):
    threshold = (lower_bound+upper_bound)/2.0
    result = 0
    while result != objective and (upper_bound-lower_bound > 0.01 or result > objective):
        threshold = (lower_bound+upper_bound)/2.0
        result = search_for_threshold_test(history, threshold)
        if result == -1:
            break
        if result > objective:
            lower_bound = threshold
        else:
            upper_bound = threshold
    if result > -1:
        print "{0}:{1}".format(history.cur_time, threshold)


def find_threshold_wrt_objective(ns_files, prev_win_size, attack_win,
        next_win_size, objective, starting_lower_bound, starting_upper_bound, filtering):
    descriptors = {}
    relays = {}
    history = History(prev_win_size, attack_win, next_win_size, descriptors,
            relays, ns_files, filtering)
    for ns_file in ns_files:
        network_state = get_n_network_states([ns_file])
        history.update_history(network_state)
        search_for_objective(history, starting_lower_bound, starting_upper_bound, objective)

def get_n_network_states(ns_files):
    network_states = []
    cons_rel_stats = {}
    for ns_file in ns_files:
        #Python fancy: I do not need to close() with this syntax
        with open(ns_file, 'r') as f:
            consensus = pickle.load(f)
            descriptors = pickle.load(f)
            hibernating_statuses = pickle.load(f)
        cons_valid_after = process_consensuses.timestamp(
                consensus.cons_valid_after)
        cons_fresh_until = process_consensuses.timestamp(
                consensus.cons_fresh_until)
        cons_bw_weights = consensus.cons_bw_weights
        missed_relays = 0
        for relay in consensus.relays:
            if relay in descriptors and descriptors[relay].write_history \
                is not None and len(descriptors[relay].write_history) > 0:
                cons_rel_stats[relay] = consensus.relays[relay]
            else:
                missed_relays += 1
                if _TESTING or _PRINT_SUSPECT:
                  print "We miss descriptor values for relay fp:{0} - name:{1}. Not added in cons_rel_stats".format(
                        relay, consensus.relays[relay].nickname)
        if _TESTING:
            print("We didn't add {0} relays over a total of {1} due to missing history".
                    format(missed_relays, len(consensus.relays)))
        network_states.append(NetworkState(cons_valid_after, cons_fresh_until, cons_bw_weights,
            hibernating_statuses, cons_rel_stats, descriptors))
    return network_states[0] if len(network_states) == 1 else network_states


#no need to use floats, sum() should not overflow 64 bits ... I hope :-)
def mean(window, slice_value):
  return sum(window.get_win_values())/(window.size/slice_value)


def variance(window, slice_value):
    m = float(mean(window, slice_value))
    var = 0.0 #set float value for unlimited Powaaaaaaaa' (to read with palpatine's voice)
    for value in window.get_win_values():
        var += (float(value)-m)**2
    return var/window.size

def is_in_range(t1, t2, t3):
  """
    if if t1 <= t2 <= t3 return True
  """
  return max(t1,min(t2, t3)) == t2


def compare_mean(r_stat, threshold, ignore_next_win):
  return mean(r_stat.prev_win, r_stat.slice_value)*threshold < \
              mean(r_stat.cur_win, r_stat.slice_value) and\
        (ignore_next_win or mean(r_stat.next_win, r_stat.slice_value)*threshold < \
              mean(r_stat.cur_win, r_stat.slice_value))

def compare_variance(r_stat, ignore_next_win, apply_variance):
  return not apply_variance or (variance(r_stat.prev_win, r_stat.slice_value) >\
          variance(r_stat.cur_win, r_stat.slice_value) and\
          (ignore_next_win or variance(r_stat.next_win, r_stat.slice_value) >\
          variance(r_stat.cur_win, r_stat.slice_value)))


def search_for_threshold_test(history, threshold):
    counter=0
    not_full = 0
    for relay in history.relays:
      r_stat = history.relays[relay]
      if r_stat.are_windows_full() and r_stat.consweight[-1] > MIN_CONSWEIGHT_CONSIDERED:
        if compare_mean(r_stat, threshold, False) and compare_variance(r_stat, False, True): #remove compare_variance to obtain results with mean only
          counter+=1
      else:
        not_full += 1
    if float(not_full)/len(history.relays) > 0.5 :
        return -1
    return counter
def apply_relaydrop_test(history, threshold, attack_started_at, ignore_next_win, apply_variance):
  for relay in history.relays:
    r_stat = history.relays[relay]
    #TODO depending of the consensus weights, find the proper threshold overhead
    if r_stat.has_new_values() and r_stat.are_windows_full() and r_stat.consweight[-1] > MIN_CONSWEIGHT_CONSIDERED:
      #if r_stat.nickname == "PiTorResque" and is_in_range(attack_started_at,\
              #r_stat.cur_win.get_data()[0][0], attack_started_at+history.cur_win_size*4*60*60):
        #pdb.set_trace()
      if attack_started_at is not None and not is_in_range(attack_started_at, r_stat.cur_win.get_data()[0][0],
              attack_started_at+(history.cur_win_size+1)*4*60*60):
        continue
      if compare_mean(r_stat, threshold, ignore_next_win) and compare_variance(r_stat, ignore_next_win, apply_variance): #remove compare_variance to obtain results with mean only
        print("Relay {0}.{1} is positive at time {2} with previous cons_weights {3} and variance {4}".format(
            r_stat.fprint, r_stat.nickname, r_stat.cur_win.get_data()[0][0],
            r_stat.consweight[len(r_stat.consweight)-(history.prev_win_size+\
            history.cur_win_size+history.next_win_size):-1], variance(r_stat.cur_win, r_stat.slice_value)))
          #print r_stat.prev_win.get_win_values()
          #print r_stat.cur_win.get_win_values()
          #print r_stat.next_win.get_win_values()
def apply_onionshape_test(history, threshold_prev, threshold_next):
  for relay in history.relays:
    r_stat = history.relays[relay]
    if r_stat.has_new_values() and r_stat.are_windows_full():
      if (sum(r_stat.prev_win.get_win_values())/
                (len(r_stat.prev_win.get_win_values())/r_stat.slice_value))*\
                (float(threshold_prev)/100) >\
                sum(r_stat.cur_win.get_win_values())/\
                (len(r_stat.cur_win.get_win_values())/r_stat.slice_value) and\
         (sum(r_stat.next_win.get_win_values())/\
                (len(r_stat.next_win.get_win_values())/r_stat.slice_value))*\
                (float(threshold_next)/100) >\
                sum(r_stat.cur_win.get_win_values())/\
                (len(r_stat.cur_win.get_win_values())/r_stat.slice_value):
        print("Relay {0}.{1} is positive. Cons weight:{2}, avg_bw{3}  at time {4}".format(r_stat.fprint,
              r_stat.nickname, r_stat.consweight, history.descriptors[relay].avg_bw, r_stat.cur_win.get_data()[0][0]))
       

if __name__ == "__main__":
  args = parser.parse_args()

  if args.subparser == "process":
    in_dirs = []
    month = args.start_month
    for year in range(args.start_year, args.end_year+1):
      while ((year < args.end_year) and (month <= 12)) or \
             (month <= args.end_month):
        if (month <= 9):
          prepend = '0'
        else:
          prepend = ''
        cons_dir = os.path.join(args.in_dir_cons, 'consensuses-{0}-{1}{2}'.\
                format(year, prepend, month))
        desc_dir = os.path.join(args.in_dir_desc,
                'server-descriptors-{0}-{1}{2}'.format(year, prepend, month))
        extra_desc_dir = os.path.join(args.in_dir_extra_desc,
                'extra-infos-{0}-{1}{2}'.format(year, prepend, month))
        desc_out_dir = os.path.join(args.out_dir,
                'network-state-{0}-{1}{2}'.format(year, prepend, month))
        if (not os.path.exists(desc_out_dir)):
          os.mkdir(desc_out_dir)
        in_dirs.append((cons_dir, desc_dir, extra_desc_dir, desc_out_dir))
        month += 1
      month = 1
    process_consensuses.process_consensuses(in_dirs, args.initial_descriptor_dir, args.initial_extrainfo_descriptor_dir)
  elif args.subparser == "pastOnions" or args.subparser == "relaydrop" or args.subparser == "thresh_variation":
      month = args.start_month
      day = args.start_day
      pathnames = []
      for year in range(args.start_year, args.end_year+1):
        while (year < args.end_year and month <= 12) or (month <= args.end_month):
          prepend_month = '0' if month <= 9 else ''
          while (year < args.end_year and month <= 12 and day <= 31) or\
            (day <= args.end_day):
            prepend_day = '0' if day <= 9 else ''
            for dirpath, dirnames, fnames in os.walk(args.in_dir):
              for fname in fnames:
                if "{0}-{1}{2}-{3}{4}".format(year, prepend_month,
                        month, prepend_day, day) in fname:
                  pathnames.append(os.path.join(dirpath, fname))
            day+=1
          day = 1
          month+=1
        month = 1
      pathnames.sort()
      filtering = args.filtering.split(',')
      for flag in filtering:
        if flag not in ['g', 'e', 'm', 'd']:
          raise ValueError("Wrong filtering: {0}".format(flag))
      if args.subparser == "pastOnions":
        lookup_pastOnions(pathnames, args.prev_win_size, args.cur_win_size, args.next_win_size,
              args.threshold_prev, args.threshold_next, filtering)
      elif args.subparser == "relaydrop":
        if args.attack_started_at is not None:
          attack_started_at = process_consensuses.timestamp(datetime.strptime(args.attack_started_at, '%Y%m%d%H').replace(tzinfo=pytz.utc))
        else:
          attack_started_at = None
        find_relaydrop_attacked_relay(pathnames, args.prev_win_size, args.attack_win,
                args.next_win_size, args.threshold, attack_started_at, filtering, args.variance)
      elif args.subparser  == "thresh_variation":
        find_threshold_wrt_objective(pathnames, args.prev_win_size, args.attack_win, args.next_win_size,
                 args.objective, args.starting_lower_bound, args.starting_upper_bound, filtering)

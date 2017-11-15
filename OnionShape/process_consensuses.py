import onionshape
import stem.descriptor.reader
from stem.descriptor.reader import FileMissing
import stem.descriptor
import stem
import os
import os.path
import cPickle as pickle
import datetime, time, pytz
import wget
import pdb
"""
This is a slightly modified version of TorPS process_consensus file. All credits to its
original author

"""
class TorOptions:
    """Stores parameters set by Tor."""
    # given by #define ROUTER_MAX_AGE (60*60*48) in or.h    
    router_max_age = 60*60*48    
    num_guards = 1
    min_num_guards = 1
    guard_expiration_min = 60*24*3600 # min time until guard removed from list
    guard_expiration_max = 90*24*3600 # max time until guard removed from list 
    default_bwweightscale = 10000   
    # max age of a dirty circuit to which new streams can be assigned
    # set by MaxCircuitDirtiness option in Tor (default: 10 min.)
    max_circuit_dirtiness = 10*60
    # max age of a clean circuit
    # set by CircuitIdleTimeout in Tor (default: 60 min.)
    circuit_idle_timeout = 60*60
    # max number of preemptive clean circuits
    # given with "#define MAX_UNUSED_OPEN_CIRCUITS 14" in Tor's circuituse.c
    max_unused_open_circuits = 14
    # long-lived ports (taken from path-spec.txt)
    # including 6523, which was added in 0.2.4.12-alpha
    long_lived_ports = [21, 22, 706, 1863, 5050, 5190, 5222, 5223, 6523, 6667,
        6697, 8300]
    # observed port creates a need active for a limited amount of time
    # given with "#define PREDICTED_CIRCS_RELEVANCE_TIME 60*60" in rephist.c
    # need expires after an hour
    port_need_lifetime = 60*60 
    # time a guard can stay down until it is removed from list    
    # set by #define ENTRY_GUARD_REMOVE_AFTER (30*24*60*60) in entrynodes.c
    guard_down_time = 30*24*60*60 # time guard can be down until is removed
    # needs that apply to all samples
    # min coverage given with "#define MIN_CIRCUITS_HANDLING_STREAM 2" in or.h
    port_need_cover_num = 2
    # number of times to attempt to find circuit with at least one NTor hop
    # from #define MAX_POPULATE_ATTEMPTS 32 in circuicbuild.c
    max_populate_attempts = 32

def timestamp(t):
    """Returns UNIX timestamp"""
    td = t - datetime.datetime(1970, 1, 1, tzinfo=pytz.UTC)
    ts = td.days*24*60*60 + td.seconds
    return ts


# def read_extra_descriptors(extra_descriptors, extra_descriptor_dir):
    # num_descriptors = 0
    # print ('Reading extra descriptors from: {0}'.format(extra_descriptor_dir))


def read_descriptors(descriptors, end_of_month_desc, descriptor_dir, skip_listener):
	"""Add to descriptors contents of descriptor archive in descriptor_dir."""
        num_descriptors = 0    
        num_relays = 0
        print('Reading descriptors from: {0}'.format(descriptor_dir))
        reader = stem.descriptor.reader.DescriptorReader(descriptor_dir, \
        validate=False)
        reader.register_skip_listener(skip_listener)
        # use read li$stener to store metrics type annotation, whione_annotation = [None]
        cur_type_annotation = [None]
        def read_listener(path):
            f = open(path)
            # store initial metrics type annotation
            initial_position = f.tell()
            first_line = f.readline()
            f.seek(initial_position)
            if (first_line[0:5] == '@type'):
                cur_type_annotation[0] = first_line
            else:
                cur_type_annotation[0] = None
            f.close()
        reader.register_read_listener(read_listener)
        with reader:
            for desc in reader:
                if (num_descriptors % 10000 == 0):
                    print('{0} descriptors processed.'.format(num_descriptors))
                num_descriptors += 1
                if (desc.fingerprint not in descriptors):
                    descriptors[desc.fingerprint] = {}
                    num_relays += 1
                    # stuff type annotation into stem object
                desc.type_annotation = cur_type_annotation[0]
                if desc.published is not None:
                    t_published =  timestamp(desc.published.replace(tzinfo=pytz.UTC))
                    if (desc.published.day > 27): #february ends the 28th and I am lazy
                        if (desc.fingerprint not in end_of_month_desc):
                            end_of_month_desc[desc.fingerprint] = {}
                        end_of_month_desc[desc.fingerprint][t_published] = desc
                    descriptors[desc.fingerprint]\
                            [t_published] = desc
        print('#descriptors: {0}; #relays:{1}'.\
            format(num_descriptors,num_relays)) 

#TODO use system call to untar after download. Python2.7 does not handle .xz compression mode
def consensus_dir_exist_or_download(in_consensus_dir):
    #pdb.set_trace()
    if not os.path.isdir(in_consensus_dir) and not os.path.exists("{0}.tar.xz".format(in_consensus_dir)):
        print "Consensus dir missing. Downloading tar file"
        wget.download("https://collector.torproject.org/archive/relay-descriptors/consensuses/{0}.tar.xz".
                format(in_consensus_dir.split('/')[-1]), "{0}.tar.xz".format(in_consensus_dir))

def process_consensuses(in_dirs, initial_descriptor_dir, initial_extra_descriptor_dir):
    """For every input consensus, finds the descriptors published most recently before the descriptor times listed for the relays in that consensus, records state changes indicated by descriptors published during the consensus fresh period, and writes out pickled consensus and descriptor objects with the relevant information.
        Inputs:
            in_dirs: list of (consensus in dir, descriptor in dir, extra descriptor in dir,
                processed descriptor out dir) triples *in order*
    """
    descriptors, end_of_month_desc = {}, {}
    extra_descriptors, end_of_month_extra_desc = {}, {}

    def skip_listener(path, exception):

        print('ERROR [{0}]: {1}'.format(path, exception))
        #TODO makes several attemps and use mirror if download fails ? 
        if isinstance(exception, FileMissing) and not os.path.exists("{0}.tar.xz".format(path)):
            path_elems = path.split('/')
            filename = path_elems[-1]
            if "extra" in filename:
                url = "https://collector.torproject.org/archive/relay-descriptors/extra-infos/{0}.tar.xz".format(filename)
            elif "server" in filename:
                url = "https://collector.torproject.org/archive/relay-descriptors/server-descriptors/{0}.tar.xz".format(filename)
            elif "consensuses" in filename:
                url = "https://collector.torproject.org/archive/relay-descriptors/consensuses/{0}.tar.xz".format(filename)
            else:
                raise ValueError("filename is not about descriptors")
            print "Downloading descriptors to {0}.tar.xz".format(path)
            wget.download(url, "{0}.tar.xz".format(path))
        
    # initialize descriptors
    if (initial_descriptor_dir is not None and initial_extra_descriptor_dir is not None):
        read_descriptors(descriptors, end_of_month_extra_desc, initial_descriptor_dir, skip_listener)
        read_descriptors(extra_descriptors, end_of_month_extra_desc, initial_extra_descriptor_dir, skip_listener)
    firstLoop = True
    for in_consensuses_dir, in_descriptors, in_extra_descriptors, desc_out_dir in in_dirs:
		# read all descriptors into memory        a
        if not firstLoop:
            descriptors = {} #free memory
            descriptors.update(end_of_month_desc)
            end_of_month_desc  = {}  #free memory
            extra_descriptors = {}
            extra_descriptors.update(end_of_month_extra_desc)
            end_of_month_extra_desc = {}
            firstLoop=False
        read_descriptors(descriptors, end_of_month_desc,  in_descriptors, skip_listener)
        # read all extra descriptors
        read_descriptors(extra_descriptors, end_of_month_extra_desc, in_extra_descriptors, skip_listener)
        # output pickled consensuses, dict of most recent descriptors, and 
        # list of hibernation status changes

        consensus_dir_exist_or_download(in_consensuses_dir)

        num_consensuses = 0
        pathnames = []
        for dirpath, dirnames, fnames in os.walk(in_consensuses_dir):
            for fname in fnames:
                pathnames.append(os.path.join(dirpath,fname))
        pathnames.sort()
        for pathname in pathnames:
            filename = os.path.basename(pathname)
            if (filename[0] == '.'):
                continue
            
            print('Processing consensus file {0}'.format(filename))
            cons_f = open(pathname, 'rb')

            # store metrics type annotation line
            initial_position = cons_f.tell()
            first_line = cons_f.readline()
            if (first_line[0:5] == '@type'):
                type_annotation = first_line
            else:
                type_annotation = None
            cons_f.seek(initial_position)

            descriptors_out = dict()
            extra_descriptors_out = dict()
            hibernating_statuses = [] # (time, fprint, hibernating)
            cons_valid_after = None
            cons_fresh_until = None
            cons_bw_weights = None
            cons_bwweightscale = None
            relays = {}
            num_not_found = 0
            num_found = 0
            # read in consensus document
            i = 0
            for document in stem.descriptor.parse_file(cons_f, validate=False,
                document_handler='DOCUMENT'):
                if (i > 0):
                    raise ValueError('Unexpectedly found more than one consensus in file: {}'.\
                        format(pathname))
                if (cons_valid_after == None):
                    cons_valid_after = document.valid_after.replace(tzinfo=pytz.UTC)
                    # compute timestamp version once here
                    valid_after_ts = timestamp(cons_valid_after)
                if (cons_fresh_until == None):
                    cons_fresh_until = document.fresh_until.replace(tzinfo=pytz.UTC)
                    # compute timestamp version once here
                    fresh_until_ts = timestamp(cons_fresh_until)
                if (cons_bw_weights == None):
                    cons_bw_weights = document.bandwidth_weights
                if (cons_bwweightscale == None) and \
                    ('bwweightscale' in document.params):
                    cons_bwweightscale = document.params[\
                            'bwweightscale']
                for fprint, r_stat in document.routers.iteritems():
                    relays[fprint] = onionshape.RouterStatusEntry(fprint, r_stat.nickname, \
                        r_stat.flags, r_stat.bandwidth)
                consensus = document
                i += 1
                            

            # find relays' most recent unexpired descriptor published
            # before the publication time in the consensus
            # and status changes in fresh period (i.e. hibernation)
            for fprint, r_stat in consensus.routers.iteritems():
                pub_time = timestamp(r_stat.published.replace(tzinfo=pytz.UTC))
                desc_time = 0
                descs_while_fresh = []
                desc_time_fresh = None
                # get all descriptors with this fingerprint
                if (r_stat.fingerprint in descriptors):
                    for t,d in descriptors[r_stat.fingerprint].items():
                        # update most recent desc seen before cons pubtime
                        # allow pubtime after valid_after but not fresh_until
                        if (valid_after_ts-t <\
                            TorOptions.router_max_age) and\
                            (t <= pub_time) and (t > desc_time) and\
                            (t <= fresh_until_ts):
                            desc_time = t
                        # store fresh-period descs for hibernation tracking
                        if (t >= valid_after_ts) and \
                            (t <= fresh_until_ts):
                            descs_while_fresh.append((t,d))                                
                        # find most recent hibernating stat before fresh period
                        # prefer most-recent descriptor before fresh period
                        # but use oldest after valid_after if necessary
                        if (desc_time_fresh == None):
                            desc_time_fresh = t
                        elif (desc_time_fresh < valid_after_ts):
                            if (t > desc_time_fresh) and\
                                (t <= valid_after_ts):
                                desc_time_fresh = t
                        else:
                            if (t < desc_time_fresh):
                                desc_time_fresh = t

                # output best descriptor if found
                if (desc_time != 0):
                    num_found += 1
                    # store discovered recent descriptor
                    desc = descriptors[r_stat.fingerprint][desc_time]
                    extra_found = False
                    try :
                        extra_descs = extra_descriptors[r_stat.fingerprint]
                        for key, extra_desc_elem in extra_descs.iteritems():
                            if desc.extra_info_digest == extra_desc_elem.digest():
                                extra_desc = extra_desc_elem
                                extra_found = True
                                break
                        if extra_found:
                            write_history = []
                            #convert to timestamp
                            if extra_desc.write_history_end is not None:
                                end_interval = (int) (time.mktime(extra_desc.write_history_end.utctimetuple()))
                                i = len(extra_desc.write_history_values)
                                for value in extra_desc.write_history_values:
                                    write_history.append((end_interval-i*extra_desc.write_history_interval, value))
                                    i -= 1
                            else :
                                print("Relay {0}.{1} has an extra_desc uncomplete: write_history_end, write_history_values: {3}".format(
                                    r_stat.fingerprint, desc.nickname,extra_desc.write_history_end, extra_desc.write_history_values))
                        else:
                            print("Relay {0}.{1} does not have extra_descriptors"\
                                .format(r_stat.fingerprint, desc.nickname))
                    except KeyError: 
                        print("Relay {0}.{1} does not have extra_descriptors"\
                                .format(r_stat.fingerprint, desc.nickname))
                    if extra_found :
                        descriptors_out[r_stat.fingerprint] = \
                            onionshape.ServerDescriptor(desc.fingerprint, \
                                desc.hibernating, desc.nickname, \
                                desc.family, desc.address, \
                                desc.average_bandwidth, desc.burst_bandwidth, \
                                desc.observed_bandwidth, \
                                desc.extra_info_digest, \
                                end_interval, \
                                extra_desc.write_history_interval, \
                                write_history)
                    else:
                        descriptors_out[r_stat.fingerprint] = \
                            onionshape.ServerDescriptor(desc.fingerprint, \
                                desc.hibernating, desc.nickname, \
                                desc.family, desc.address, \
                                desc.average_bandwidth, desc.burst_bandwidth, \
                                desc.observed_bandwidth, \
                                desc.extra_info_digest, \
                                None, None, None)
                     
                    # store hibernating statuses
                    if (desc_time_fresh == None):
                        raise ValueError('Descriptor error for {0}:{1}.\n Found  descriptor before published date {2}: {3}\nDid not find descriptor for initial hibernation status for fresh period starting {4}.'.format(r_stat.nickname, r_stat.fingerprint, pub_time, desc_time, valid_after_ts))
                    desc = descriptors[r_stat.fingerprint][desc_time_fresh]
                    cur_hibernating = desc.hibernating
                    # setting initial status
                    hibernating_statuses.append((0, desc.fingerprint,\
                        cur_hibernating))
                    if (cur_hibernating):
                        print('{0}:{1} was hibernating at consenses period start'.format(desc.nickname, desc.fingerprint))
                    descs_while_fresh.sort(key = lambda x: x[0])
                    for (t,d) in descs_while_fresh:
                        if (d.hibernating != cur_hibernating):
                            cur_hibernating = d.hibernating                                   
                            hibernating_statuses.append(\
                                (t, d.fingerprint, cur_hibernating))
                            if (cur_hibernating):
                                print('{0}:{1} started hibernating at {2}'\
                                    .format(d.nickname, d.fingerprint, t))
                            else:
                                print('{0}:{1} stopped hibernating at {2}'\
                                    .format(d.nickname, d.fingerprint, t))                   
                else:
#                            print(\
#                            'Descriptor not found for {0}:{1}:{2}'.format(\
#                                r_stat.nickname,r_stat.fingerprint, pub_time))
                    num_not_found += 1
                    
            # output pickled consensus, recent descriptors, and
            # hibernating status changes
            if (cons_valid_after != None) and\
                (cons_fresh_until != None):
                consensus_out = onionshape.NetworkStatusDocument(\
                    cons_valid_after, cons_fresh_until, cons_bw_weights,\
                    cons_bwweightscale, relays)
                hibernating_statuses.sort(key = lambda x: x[0],\
                    reverse=True)
                outpath = os.path.join(desc_out_dir,\
                    cons_valid_after.strftime(\
                        '%Y-%m-%d-%H-%M-%S-network_state'))
                f = open(outpath, 'wb')
                pickle.dump(consensus_out, f, pickle.HIGHEST_PROTOCOL)
                pickle.dump(descriptors_out,f,pickle.HIGHEST_PROTOCOL)
                pickle.dump(hibernating_statuses,f,pickle.HIGHEST_PROTOCOL)
                f.close()

                print('Wrote descriptors for {0} relays.'.\
                    format(num_found))
                print('Did not find descriptors for {0} relays\n'.\
                    format(num_not_found))
            else:
                print('Problem parsing {0}.'.format(filename))             
            num_consensuses += 1
            
            cons_f.close()
                
        print('# consensuses: {0}'.format(num_consensuses))

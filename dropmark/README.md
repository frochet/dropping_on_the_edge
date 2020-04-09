

# Experimentation regarding section dropmark and flow fingerprint:


This repository holds code and procedure to reproduce the results in Figures from
paper "Dropping at the Edge: Flexibility and Traffic Confirmation in
Onion Routing Protocols" and numerical results from the paper

## Install

- Follow shadow/README.md Quick Setup and
  https://github.com/shadow/shadow/wiki/1-Installation-and-Setup
- Follow shadow-plugin-tor/README.md for Quick setup. Build with
  --tor-prefix towards the source code of the modified
  tor given in this directory
- Careful, you may want to build tor with --openssl-prefix and
  --libevent-prefix with stable of version from 2018. There is no
guarantee it will build with you current dynamic library.
- Download the appropriate decriptors on the CollectTor website


## Density of connected cells and first data cell


- Considering that we are inside shadow-plugin-tor, do mkdir
  your_sim_dir_name && cd your_sim_dir_name  
- Generate the topology:  

```bash
python ../tools/generate.py --nauths 3  --nrelays 50 --nclients 200 --nservers 20 --fweb 1.0 --fbulk 0.0 ../alexa-top-1000-ips.csv ../descriptors/2017-03-30-23-00-00-consensus ../descriptors/server-descriptors-2017-03/ ../descriptors/extra-infos-2017-03/ ../clients.csv
```

/!\ The load balancing weights works correctly as long as we have at
least 3 authorities in the testing network:
http://mailman.cs.umn.edu/archives/shadow-support/2015-November/000462.html


- In your topology, modify the Torrc of exit relay and add
  SignalLogEachRelayedCellTiming 1  
  Modify also in the common torrc file the Log line to: Log [signal]info
stdout
- Run the simulation: 

```bash
~/.shadow/bin/shadow-tor -i shadow.config.xml &
```
- Get the figure by running the python script:
  
```bash
 python parse_and_plot.py webbarchart shadow-plugin-tor/small_network/shadow.data/hosts/relayexit1/stdout-tor-1000.log relayexit1
```

- You may choose another exit relay. The result
  should look the same

## Dropmark experiment


4 experimentations in totals, 2 with light loaded network and 2 with
normal loaded network (similar to the real Tor network)  

### Light load: dropmark not sent but decoding function active

- Generate the topology:  

```bash
 python ../tools/generate.py --nauths 3 --nrelays 50 --nclients 200 --nservers 20 --fweb 1.0 --fbulk 0.0 ../alexa-top-1000-ips.csv ../descriptors/2017-03-30-23-00-00-consensus ../descriptors/server-descriptors-2017-03/ ../descriptors/extra-infos-2017-03/ ../clients.csv
```

% TODO: modify the generate.py script to automatize the torrc edit

- Edit the torrc files in conf/  

   1. **tor.client.torrc**

      ORPort 0  
      DirPort 0  
      ClientOnly 1  
      SocksPort 127.0.0.1:9000 IsolateDestAddr  
      BandwidthRate 5120000  
      BandwidthBurst 10240000  
      NewCircuitPeriod 1  
      UseEntryGuardsAsDirGuards 0  

   2. **tor.common.torrc**  
      Following lines must appear:  
      Log [signal]info stdout  
      MaxCircuitDirtiness 1  
      SignalMethod 2  
      SignalBlankIntervalMS 5

   3. **tor.guard.torrc**

      ORPort 9111  
      SocksPort 0  
      DirPort 0  
      ExitPolicy "reject \*:\*"  
      ActivateSignalAttackListen 1  

   4. **tor.exit.torrc** and **tor.exitguard.torrc**

      ORPort 9111  
      SocksPort 0  
      DirPort 0  
      ExitPolicy "accept \*:80"  
      ExitPolicy "reject \*:\*"  

   5. **tor.middle.torrc**

      ORPort 9111  
      SocksPort 0  
      DirPort 0  
      ExitPolicy "reject \*:\*"  

- Run the simulation (it takes a few hours)


- Now, you have in shadow.data/hosts/relayguard\*/stdout-tor-1000.log the
  logs of all tested circuits over the dropmark attack. To check the
  number of false positives, do:

```bash
  $ grep "Spotted watermark" shadow.data/hosts/relayguard*/stdout-tor-1000.log | wc -l
```
- In order to know the number of errors/successful transfers, do:  

```bash
  $ grep transfer-error shadow.data/hosts/webclient*/stdout-tgen-1002.log | wc -l
```  

```bash
  $ grep transfer-complete shadow.data/hosts/webclient*/stdout-tgen-1002.log | wc -l
```

### Normal load: dropmark not sent but decoding function active

- Generate the topology using 450 web clients instead of 200.

- Edit the torrcs like in the previous section.

### Light load: dropmark sent and decoding function active

- Create a new directory, cd in and generate the topology:  

```bash
 python ../tools/generate.py --nauths 3 --nrelays 50 --nclients 200 --nservers 20 --fweb 1.0 --fbulk 0.0 ../alexa-top-1000-ips.csv ../descriptors/2017-03-30-23-00-00-consensus ../descriptors/server-descriptors-2017-03/ ../descriptors/extra-infos-2017-03/ ../clients.csv
```
- Edit the torrc files in conf/  

   1. **tor.client.torrc**  
  
      ORPort 0  
      DirPort 0  
      ClientOnly 1  
      SocksPort 127.0.0.1:9000 IsolateDestAddr  
      BandwidthRate 5120000  
      BandwidthBurst 10240000  
      NewCircuitPeriod 1  
      UseEntryGuardsAsDirGuards 0  

   2. **tor.common.torrc**
      Following lines must appear  
      Log [signal]info stdout  #(remove other Log line)  
      MaxCircuitDirtiness 1  
      SignalMethod 2  
      SignalBlankIntervalMS 5  
  
   3. **tor.guard.torrc**
    
      ORPort 9111  
      SocksPort 0  
      DirPort 0  
      ExitPolicy "reject \*:\*"  
      ActivateSignalAttackListen 1  
   
   4. **tor.exit.torrc** and **tor.exitguard.torrc**
    
      ORPort 9111  
      SocksPort 0  
      DirPort 0  
      ExitPolicy "accept \*:80"  
      ExitPolicy "reject \*:\*"  
      ActivateSignalAttackWrite 1  

/!\ The exit relays send a dropmark for all circuits that connect to
addresses specified by following lines:  
     **WatchAddres IP**  
Where IP is the IP address of one of the web servers. You need to put 20
lines with the 20 IPs from the web servers. /!\  
One problem though, if the generate.py script created a config with same
IPs on multiple servers (look in shadow.config.xml), then some servers will
take a different IP from the one indicated in the config file. Run once
the simulation, kill it after a few minutes and check in shadow.log the
ips of the servers :

```bash
grep server shadow.log | tail -n 20
```
This should give you the true 20 IPs of the servers. Then edit the Torrc
file and clean the simulation directory:
```bash
rm -rf shadow.data && rm shadow.log
```

   5. **tor.middle.torrc**

      ORPort 9111  
      SocksPort 0  
      DirPort 0  
      ExitPolicy "reject \*:\*"  

- Run the simulation (few hours)
- Now you have in shadow.data/hosts/relayguard\*/stdout-tor-1000.log the
  logs of all tested circuits. Look for the number of false negatives
by:  

```bash
grep "No watermark" shadow.data/hosts/relayguard*/stdout-tor-1000.log | wc -l
```

- Number of true positives:  

 ```bash
grep "Spotted watermark" shadow.data/hosts/relayguard*/stdout-tor-1000.log | wc -l
 ```  

- You can also look into the number of begin cells sent by webclients and
  the number of signal send by exits. Do a grep over their log
files and count lines as above. These numbers will be higher than the
number of circuits seen by the guards because:  
  1. Sometimes the exit timeout and a new begin is sent along another
     Tor circuit  
  2. Sometimes some begin cells not sent over a fresh circuit despite
     the MaxCircuitDirtiness set to 1 second. Hence, those dropmarks are
not analyzed  (but not counted as a false negatives, since we do not
try to decode them). We could add the possibility of decoding multiple
dropmarks per circuit but it would be for worthless purpose from a
research point of view. However, that's what a true adversary would like
to have in practice.  

### Normal load: dropmark sent and decoding function active

- Generate a topology with 450 webclients instead of 200 and do the same
  steps as previous section.

### From dropmark to flow fingerprint (section 6.2)

In this part, we validate the approach of delaying the destroy cell
and sending the fingerprint when the connection between the victim and
the colluding guard has died. From the exit perspective, we cannot be sure that the client closed
the circuit but, after 10 seconds, we already have a probability higher than 80\% that the
connection is closed (See Kwon paper at Usenix 2015).  
This experiment does not aim at successfully decoding the fingerprint on the guard
position. This can be efficiently done using code-correctors algorithms,
as pointed out by previous works.
At least, we show that it **can** be done in a complete stealthy way.

- Create a topology and edit torrcs files. Small differences with the
  privous section: the **tor.common.torrc** file must contain SignalMethod 0
and SignalBlankIntervalMS 50 (or more. Remember: we don't care about
amplitude :-).
- Guard relays delay the destroy cell by SignalLaunchDelay+60
  seconds with SignalLaunchDelay 30 by default. Exit relays send the
signal SignalLaunchDelay seconds after the dns_resolve().

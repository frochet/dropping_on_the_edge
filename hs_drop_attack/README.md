# Hidden Service Guard Overload Program

## Description  

This software has been used to provide results published in paper
"Dropping on the Edge: Flexibility and Traffic Confirmation in Onion
Routing Protocols". The goal of this software is to overload the Guard used by an
Onion Service, given its onion address. Two parameters are needed  

- The onion address of the targeted service  
- The time when the attack must be stopped  

The python script acts as an interface between the user and the
(modified) Tor software. It launches the Tor software,  sends requests to Tor and get responses.

## Build and set up

Compile the Tor software provided here:

```bash
cd ../tor  
git checkout hs_RD_attack //switch from master to our modified Tor
version  
autoreconf -i install  
./configure --prefix /path/to/your/install/dir  //if it fails, verify
dependy requirements  
make && make install  
```
Go back to hs_attack directory. 
 
Now you can open the python file hs_rd_attack.py and modify the
DataDirectory variable (stores Tor related files) and the Tor binary
path.  
Change also the config to what you want (bottom of the file).  

### Python dependencies:

-txtorcon, twisted

## Use: example  

### On the real Tor network  

```bash
python hs_rd_attack.py --onion onion_address_file --nbrcircuits 50 --until 2016-12-22-10-00 --bandwidth 5000
```

This line should inject 5 MB/s of relay drop cells
towards the onion address inside the onion_address_file, through 50
circuits and until YYYY-MM-DD-hh-mm datetime indicated.

### On a private Tor network using chutney  

A network topology is configured with a low-bandwidth onion service
allowing us to test the counter weakness described in the paper.

Chutney might be configured with whatever recent Tor binary (follow its
Readme). It might be preferable to configure different values for
bandwidth reporting interval than the default 4 hours, in order to test the attack
in a few minutes instead than in several hours. Modify
WRITE_STATS_INTEVAL, NUM_SECS_BW_SUM_INTERVAL and
NUM_SECS_BW_SUM_IS_VALID to something small. For example: 2\*60, 60 and
60\*60. Recompile the Tor source code  and you're ready.  

in the chutnet directory, do:  

```bash
./chutney configure networks/hs_attack
./chutney start netwoks/hs_attack
```
Retrieve the onion address:

```bash
cat net/nodes.[timestamp]/[hs_dir]/hidden_service/hostname >
../client_side/chutney_onion
```

Then, you can launch the relay drop attacks against your private
network:  
 
```bash
python hs_rd_attack.py --onion onion_address_file --nbrcircuits 50 --until 2016-12-22-10-00 --bandwidth 5000 --chutneynodepath [you/path/to/nodes.[timestamp]]
```
## Bugs

- Time is not in utc. Some bugs might happen when the timestamp is converted
  in Tor, depending on your localtime (needs to be fixed).  

## Improvements

- Handle timeouts w.r.t to an estimation of the HS Guard congestion.


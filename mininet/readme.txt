1) If you don't have script to convert you can skip the conversion

########################################################################################
#                	     Conversion Clients' Script		         	   #
########################################################################################

1) Add this code at the beginning of the script :
EX_NAME=$(pwd | awk -F "/" '{print $NF}')


2) After the while on the input parameter add this code and comment the following line:

export CCND_LOG=./log.$EX_NAME
export CCN_LOCAL_SOCKNAME=/tmp/.sock.ccnx.$EX_NAME
#kill $(pidof ccnd) > /dev/null 2>&1

3) Modify the name of the executable :

#$BASE_DIR/csrc/ccnd/ccnd 1 1 0 0 0  > /dev/null 2>&1 &
./$EX_NAME-ccnd 1 1 0 0 0  > /dev/null 2>&1 &

4) Change BaseDir considering that execution start from ./alien-ofelia-ccnx/mininet/cliX
../../../alien-ofelia-conet-ccnx

5) Copy the script in the folder install/client

6) After the execution of the script start_experiment.py check and set the server ip 
and the contents in accord to your experiment


########################################################################################
#   		             Conversion Servers' Script		                  #
########################################################################################
  
1) Add this code at the beginning of the script (as in client) :
EX_NAME=$(pwd | awk -F "/" '{print $NF}')

2) After the while on the input parameter add this code:

export CCND_LOG=./log.$EX_NAME
export CCN_LOCAL_SOCKNAME=/tmp/.sock.ccnx.$EX_NAME

3)Modify the name of the executable :

#$BASE_DIR/csrc/ccnd/ccnd 0 0 0 0 0  > /dev/null 2>&1 &
./$EX_NAME-ccnd 0 0 0 0 0  > /dev/null 2>&1 &

4)Change the repo dir:

#cd $BASE_DIR/$REP_DIR
cd ./repo

5) Change BaseDir considering that execution start from ./alien-ofelia-ccnx/mininet/serX/repo
../../../../alien-ofelia-conet-ccnx

6) Copy the script in the folder install/server

7) After the execution of the script start_experiment.py check 
and set the contents in accord to your experiment

##########################################################################################

3) Run the script start_experiment in this manner: sudo ./start_experiment -t /--topo=[toponame] -c/--compile -v/--verbose.
	- topo name can be (for now): fattree , i2cat, multisitem, multisitel, multisitexl, mesh[x];
	- -c/--compile the system compiles alien-ofelia-ccnx and generate the executable; 
	- -v/--verbose debug mode

4) Change the contents name in the server host and server ip in host client in order
to mee your configuration;

5) Start the controller;

6) Start the server;

7) Start the cache server;

8) Start the client when the server finished to publish the contents

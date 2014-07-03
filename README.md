alien-ofelia-conet-ccnx
=======================

alien-ofelia cooperation conet-ccnx source code

Conet Usage
=======================

See [alien-ofelia-readme](https://www.dropbox.com/s/0wghg72du1frodx/alien-ofelia-testbed.txt)

Mininet Usage
=======================

- Run the script start_experiment.py:
		~/alien-ofelia-conet-cnnx/mininet$ sudo ./start_experiment -t /--topo=[toponame] -c/--compile -v/--verbose.

		-topo			topology to deploy: fattree , i2cat, multisitem, multisitel, multisitexl, mesh[x];

		-c/--compile	the system compiles alien-ofelia-ccnx and generate the executable; 

		-v/--verbose	debug mode

- At the end of this step you should see the mininet prompt ( mininet>), otherwise there are some problems with your configuration and our dependecies.

- If you can see the mininet prompt the next step is to open the associated xterm of each host: 
		xterm cli1 cli2 ser1 ser2 cse1 cse2

- Enter in the folder of each host, for example:
		cd cli1/ 

- In the folder of each host there are scripts useful to test our solution. You can find them in the host folders:
		singleserver - one content is published by the server
		singleclient - one content is requested by the client
		
		manyserver_* - we have implemented different behavior, but the basic idea is that the server publishs more than one content
		manyclient_* - In this case the client requests more than one content

- These scripts are coupled because the client should request the content published, otherwise the request cannot work properly. You have to check that the contents name corresponds on both side.

- The clients script needs the server ip. You can change it in the client's scripts. 

- Start our custom implementation of Floodlight Controller see [Floodlight implementation for Conet](https://github.com/StefanoSalsano/my-floodlight/tree/0.90).

- Start the server running one of the built-in scripts or your own script (properly configured). You should be able to see the publishing of the contents.
		~ser1$ ./manyserver -n server

- Start the cache server running the script start.sh. You should able to see the cache server that connects to the controller and the cache server should receive a periodic hello message from the controller.
		~cse1$ ./start.sh

- Start the client running one of the built-in scripts or you own (At the end of content publishing). You should be able to see the requests of the content.
		~cli1$ ./manyclient -n client

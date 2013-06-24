*******************
*** PRELIMINARIES ****
*******************
- Install sudo
	apt-get install sudo

- Setup the sudoers
	su
	nano /etc/sudoers
On section "# User privilege specification", after ROOT line, insert the following
	<username1>    ALL=(ALL) ALL
	<username2>    ALL=(ALL) ALL
	...

- Install subversion
	apt-get update
	apt-get install subversion
	
- Set environment variables
	- open <your_home_directory>/.bashrc and add the following:
		LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<absolute_path_of_cache_server_fodler>//conetImpl/lib
		export LD_LIBRARY_PATH


- Install libpcap (TODO: forse superfluo)
	apt-get install libpcap-dev

*******************
*** BUILDING ****
*******************
- cd <cache_server_folder>
- sudo svn update
- sudo nano csrc/include/conet/conet.h and do the following checks:
	- there must be the following line
		#define IS_CACHE_SERVER 1
	- #define IFNAME must be set according to the results of ifconifig output
- sudo sh make_all.sh


*******************
*** RUNNING ****
*******************
- sudo sh go_listener.sh (if you wnat to change some parameters, you have to modify this script first)


















VECCHIUME

!!! CAUTION !!!: you have to install oracle-java (and not jdk)

If you have some problem, please check the section "TROUBLESHOUTING" at the end of this document


Please, make sure your $JAVA_HOME is correctly set (for example in your "<home>/.bashrc" file) before continuing.



Please, add also this line:
	alias sudo='sudo env JAVA_HOME=$JAVA_HOME'
at the end of your "<home>/.bashrc" file. This will ensure your $JAVA_HOME will be correctly set when in sudo-mode as well.


//TODO: vedere se si puo' evitare sta seccatura mediante compilazione statica
Before working, please set also the environment variable LD_LIBRARY_PATH in the following way:
	        LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<folder_of_your_cache_engine>/Listener10.04/conetImpl/lib



For example you can put this lines at the end of your .bashrc

	JAVA_HOME=/ofelia/users/andreaaraldo/installed/java/jdk1.7.0_03
	PATH=$PATH:$HOME/bin:$JAVA_HOME/bin
	export JAVA_HOME
	export JAVA_BIN
	export PATH
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/ofelia/users/andreaaraldo/v08/cacheEngine-remoto/Listener10.04/conetImpl/lib 
	export LD_LIBRARY_PATH
	alias sudo='sudo env JAVA_HOME=$JAVA_HOME LD_LIBRARY_PATH=$LD_LIBRARY_PATH'


The first time you run this program on your machine, please cd to the "Listener29.03" and run this script
	sudo build_externals

Every time you edit some source file, you have to build again: cd to the "Listener29.03" folder and run:
	make_all.sh


***************************
*** SUGGESTED SCENARIO ****
***************************
We suggest the following scenario to test the implemented functionalities.
Have two hosts: PC_CACHE and a PC_CLIENT connected by their eth interfaces

On PC_CACHE run the following commands on separate terminals (the order is important here!):
- Launch the server
	- cd to the "Server01.04" folder
	- java Server
	
- Launch cacheEngine
	- cd to the "Listener29.03" folder
	- run_cache_engine.sh
(now cacheEngine is ready to work, but it still needs his front end (i.e. the Listener) )

- Launch Listener
	- sudo go_listener <interface>
(where <interface> could be eth0)


Now all the components on PC_CACHE are connected to each other. Listener can talk to Server and cacheEngine and is ready to receive interestsCIU or dataCIU from the PC_CLIENT


On PC_CLIENT, run the following
	- sudo sh send_dataCIU.sh <nid> <csn> <data> <cacheIpAddr>


*******************
*** dove sono i chunks? ***
*******************
I chunks si trovano nella cartella Listener29.03/files/chunks.
Per esempio, il chunk con csn "a33" (in esadecimale) del contenuto con nid "nome_contenuto" non e' altro che il file Listener08.02/files/chunks/nome_contenuto/a33.chu.
Attenzione: il <csn> passato nello script "newconet_send_and_receive.sh" è in forma numerica mentre il nome dei file e' in forma esadecimale. Ad esempio se si cerca un contenuto con <csn>=170 il file che si andra' a cercare e' aa.chu





##################################
###TROUBLESHOOTING#################
##################################

On 64 bits architectures, when launching sh make_all.sh, the following error may arise:

###############
##### ERROR 1##
###############

/usr/bin/ld: <your_gcc_path>/crtbeginT.o: relocation <.....>

To solve this error, 
	- cd to <your_gcc_path>
	- cp crtbeginT.o crtbeginT.o.originale
	- cp crtbeginS.o crtbeginT.o

(see osdir.com/ml/ubuntu-bugs/2011-05/msg06162.html)


Launch sh make_all.sh again

###############
##### ERROR 2##
###############
/usr/bin/ld: ./conetImpl/lib/libconet.a(cacheEngine.o): relocation R_X86_64_32 against `.text' can not be used when making a shared object; recompile with -fPIC

(or similar)

To solve this error:
	- open "conetImpl/makefile"
	- at the end of every gcc line, insert
			-fPIC
	- do the same modification on the file "make_raw.sh"
	- launch sh make_all.sh again

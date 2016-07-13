#!/bin/bash

example() {


	ccn_pids=$( ps ax |grep ccn | awk {'print $1'})

	for pid in ${ccn_pids[@]};
        do
		echo $pid
		sudo kill $pid &> /dev/null
	done

	sudo killall start_mininet.py &> /dev/null
	sudo killall java &> /dev/null
	sudo killall python &> /dev/null
	killall firefox &> /dev/null
	sudo stop network-manager &> /dev/null
	
	xfce4-terminal -T mininet -e 'env PROMPT_COMMAND=" 
	unset PROMPT_COMMAND
	history -s sudo ./start_mininet.py -t i2cat
	sudo ./start_mininet.py -t i2cat" bash' --working-directory=/home/conet/alien-ofelia-conet-ccnx/mininet &
	
	sleep 3	
	
	xfce4-terminal -T controller -e 'env PROMPT_COMMAND="
	unset PROMPT_COMMAND
	history -s ./start.sh
	./start.sh" bash' --working-directory=/home/conet/my-floodlight &

	sleep 3
 	sudo python /opt/lampp/htdocs/mrtg/cacheditems.py i2cat &	
	#xfce4-terminal -T items -e 'env PROMPT_COMMAND="
        #unset PROMPT_COMMAND
        #history -s sudo /opt/lampp/htdocs/mrtg/cacheditems.py i2cat
        #sudo /opt/lampp/htdocs/mrtg/cacheditems.py i2cat" bash'  &

	sleep 3

	firefox
}

$1


#!/bin/bash

#Correct usage 
#	conetImpl/cacheEngine.o <cache server IP> <cache server mac addr> <controller ip> <controller port>

export LD_LIBRARY_PATH="/root/alien-ofelia-conet-ccnx/cache_server/conetImpl/lib"
rm -R files/chunks/*

valgrind conetImpl/lib/listener.o  192.168.1.248    02:02:21:11:11:6b    10.216.33.109   9999
#valgrind conetImpl/lib/listener.o  192.168.1.218    02:03:00:00:02:44    10.216.12.88   9999

# > cache.log 2>&1



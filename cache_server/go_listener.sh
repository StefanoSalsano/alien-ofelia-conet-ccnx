#Correct usage 
#	conetImpl/cacheEngine.o <cache server IP> <cache server mac addr> <controller ip> <controller port>

rm -R files/chunks/*

valgrind conetImpl/lib/listener.o  192.168.1.83    02:03:00:00:00:b0    10.216.12.88   9999

# > cache.log 2>&1



#!/usr/bin/python

import subprocess
from time import time

SERVING_NODE_IP = "10.0.0.2"
#"192.168.1.9"
SERVING_NODE_CONET_PORT = "9698"
CCN_URI = "ccn:/prova/1mb"
FILE_SIZE = 1024*1024*1*8
CATCHUNK_PIPE_LIMIT = "25"
sommaGP=0
sommaT=0
i=0
maxGP=0
minGP=1000000000000000
prec=10000000
test_cases = ["ccnx", "conet+p", "conet"]
base_name = "/home/cancel/Documenti/tesi/misure/noloss"
results = []
use_conet="0"
conet_prefetch="0"
for t in test_cases:
	print("_________" + t + "________")
	if (t=="ccnx"):
		use_conet="0"
	else:
		use_conet="1"
	if (t=="conet+p"):
		conet_prefetch="1"
	else:
		conet_prefetch="0"
	print(use_conet + "_____" + conet_prefetch)
	out_file=open(base_name + "_" + t + ".csv", "w")
	out_file.write(t + "\n")
	out_file.write("#RUN, GOODPUT, TIME\n");
	i=0
	while (i<10):
		
		proc = subprocess.Popen(["./../ccnd/ccnd", use_conet, conet_prefetch])
		start=time()
		end=time()
		while (end-start<5):
			end=time()	
		if (t!="ccnx"):
			ret0 = subprocess.call(["ccndc","add", "ccn:/prova", "udp", SERVING_NODE_IP, SERVING_NODE_CONET_PORT])
		ret1 = subprocess.call(["ccndc","add", "ccnx:/", "udp", SERVING_NODE_IP])
		t0 = time()
		ret2 = subprocess.call(["./ccncatchunks2","-p", CATCHUNK_PIPE_LIMIT, "-s", CCN_URI])
		t1 = time()
		proc.kill()
		if (use_conet=="1"):
			print(ret0)
		print(ret1)
		print(ret2)
		if (ret1==0 ):
			gp=FILE_SIZE/(t1-t0)
			rt=t1-t0
			i = i+1
			print ('TIME: %f' %(t1-t0))
			print ('GOODPUT: %f' %(FILE_SIZE/(t1-t0)))
			out_file.write(str(i) + ", " + str(gp) + ", " + str(rt) + "\n")
			out_file.flush()

	
	out_file.close()






	
	

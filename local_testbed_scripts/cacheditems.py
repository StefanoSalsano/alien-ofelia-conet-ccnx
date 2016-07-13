#!/usr/bin/python
import sys, time
import json, datetime
import os, errno
import subprocess
import json, datetime
import urllib2
import rrdtool
import shutil

def create_rrd_path(path):
	if not os.path.exists(path):
		os.makedirs(path)
		os.chmod(path0, 0777)
	try:
                rrdtool.create(path +"/cacheditems.rrd", "--step", "10", "--start", "0","DS:cached_items:GAUGE:20:0:10000","RRA:AVERAGE:0.5:1:360","RRA:AVERAGE:0.5:20:432")
	except OSError as exception:
		if exception.errno != errno.EEXIST:
			raise

mrtg_path = '/opt/lampp/htdocs/mrtg/'

#testbed = 'multisite'
#switches = ['00.10.00.00.00.00.00.03','00.20.00.00.00.00.00.03','00.20.08.00.00.00.00.03']

testbed = 'i2cat'
switches = ['00.10.00.00.00.00.00.03']

create_rrd_path(mrtg_path + 'controller_')
create_rrd_path(mrtg_path + testbed)

for switch in switches:
	create_rrd_path(mrtg_path + testbed +"/" + switch)

while(1):
	j = urllib2.urlopen('http://127.0.0.1:8080/icn/cache-server/all/cacheditemsmap/json')
	o = json.loads(j.read())
	total = 0
	for key in o.keys():
		if key=='0010000000000003':
			total += o[key]
			rrdtool.update(mrtg_path + testbed + '/00.10.00.00.00.00.00.03/cacheditems.rrd','N:' + str(o[key]));
		elif key=='0020000000000003':
                        total += o[key]
                        rrdtool.update(mrtg_path + testbed + '/00.20.00.00.00.00.00.03/cacheditems.rrd','N:' + str(o[key]));
                elif key=='0208020800000003':
                        total += o[key]
                        rrdtool.update(mrtg_path + testbed + '/02.08.02.08.00.00.00.03/cacheditems.rrd','N:' + str(o[key]));
		else:
			print "@@@@@@@@@@", key
	rrdtool.update(mrtg_path + 'controller_' + '/cacheditems.rrd','N:' + `total`);
	time.sleep(10)


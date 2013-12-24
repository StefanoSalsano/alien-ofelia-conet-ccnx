#!/usr/bin/python

# Create one topology, you can chose your topology with cmd line args
# In this file we provide 5 examples of slice:
# 1) FatTree
# 2) Siracusano' s Slice
# 3) Rashed' s Slice
# 4) Ventre' s Slice
# 5) MultiSite XLarge (it contains the resources of TUB's Island and is built on top of Ventre's Slice)
# You can use only one slice at time,
# You can run the experiment with the command: sudo ./start_mininet.py name_in_file
# ps: for name_in_file sees the name near topos

import sys
import subprocess
import time
import shutil
import os

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import lg
from mininet.node import RemoteController, Node, OVSKernelSwitch # We use OVSKernelSwitch
from mininet.link import Link

def connectToRootNS( net, ip='10.123.123.1', mac='00123456789A', prefixLen=8, routes=['10.0.0.0/8']):
	print "*** Creating controller"
	c0 = net.addController( 'c0', ip='127.0.0.1', port=6633 )
	rootswitch = net.addSwitch('roots1',)
	rootswitch.dpid = 'FFFFFFFFFFFFFFFF'
	# Connect hosts to root namespace via switch. Starts network.
	# network: Mininet() network object
	# ip: IP address for root namespace node
	# prefixLen: IP address prefix length (e.g. 8, 16, 24)
	# routes: host networks to route to"
	# Create a node in root namespace and link to switch 0
	root = Node( 'root', inNamespace=False )
	intf = Link( root, rootswitch ).intf1
	intf.setMAC(mac)
	root.setIP( ip, prefixLen, intf )
	# Start network that now includes link to root namespace
	for host in net.hosts:
	    net.addLink(host,rootswitch)
	# Add routes from root ns to hosts
	for route in routes:
         root.cmd( 'route add -net ' + route + ' dev ' + str( intf ) )
         return rootswitch
         
def FatTree():                                                                
        "Create FatTree topo."
	print "*** FatTree With 4 Client, 4 Server, 4 CacheServer"
        # Create hosts
        cli = []
        ser = []
        cse = []
        swi = []
       
        net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )
	print "*** Creating Clients"
	for h in range(0, 4):
		 cli.append(net.addHost('cli%s' % (h+1)))
	print "*** Creating Servers"
	for h in range(0, 4):
		 ser.append(net.addHost('ser%s' % (h+1)))
	print "*** Creating CacheServers"
	for h in range(0, 4):
		 cse.append(net.addHost('cse%s' % (h+1)))
        print "*** Creating Switches"
        for s in range(0, 7):
                swi.append(net.addSwitch('s%s' % (s+13)))
                swi[s].dpid = '00000000000000%s' % (s+13) # Change this line with more sws, for our scope is ok

	connectToRootNS(net)
        print "*** Connect Hosts to Switches" 
        for h in range(0,4):
	    net.addLink(cli[h], swi[h])
	    net.addLink(ser[h], swi[h])
	    net.addLink(cse[h], swi[h])
	
	print "*** Configure Clients"
	for h in range(0,4):
		 cli[h].cmd('modprobe 8021q')
		 cli[h].cmd('vconfig add %s-eth1 3001' % cli[h].name)
		 cli[h].cmd('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
		 cli[h].cmd('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
		 cli[h].cmd('ip link set %s-eth1 up' % cli[h].name)
		 cli[h].cmd('ip link set %s-eth1.3001 up' % cli[h].name)
	print "*** Configure Servers"	 
	for h in range(0,4):
		 ser[h].cmd('modprobe 8021q')
		 ser[h].cmd('vconfig add %s-eth1 3001' % ser[h].name)
		 ser[h].cmd('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
		 ser[h].cmd('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
		 ser[h].cmd('ip link set %s-eth1 up' % ser[h].name)
		 ser[h].cmd('ip link set %s-eth1.3001 up' % ser[h].name)
	print "*** Configure CacheServers"	 
	for h in range(0,4):
		 cse[h].cmd('modprobe 8021q')
		 cse[h].cmd('vconfig add %s-eth1 3001' % cse[h].name)
		 cse[h].cmd('ip addr add 192.168.128.%s/16 brd +dev %s-eth1.3001' %((h+1),cse[h].name))
		 cse[h].cmd('ip addr add 172.16.128.%s/16 brd +dev %s-eth1.3001' %((h+1),cse[h].name))
		 cse[h].cmd('ip link set %s-eth1 up' % cse[h].name)
		 cse[h].cmd('ip link set %s-eth1.3001 up' % cse[h].name)
	
        print "*** Connect Switches To Switches"
        net.addLink(swi[0], swi[4])
        net.addLink(swi[1], swi[4])
        net.addLink(swi[2], swi[5])
        net.addLink(swi[3], swi[5])
        net.addLink(swi[4], swi[6])
        net.addLink(swi[5], swi[6])
	
	return net
	
def i2CatNet():
    "i2Cat Topology Example"
    h = 0
    print "*** i2Cat Topology With 1 Client, 1 Server, 1 CacheServer"
    net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )
    print "*** Creating Client"
    cli = net.addHost('cli%s' % (h+1))
    print "*** Creating Server"
    ser = net.addHost('ser%s' % (h+1))
    print "*** Creating CacheServers"
    cse = net.addHost('cse%s' % (h+1))
    print "*** Creating Switches"
    iC3 = net.addSwitch('iC3')
    iC3.dpid='0001000000000003'
    iC1 = net.addSwitch('iC1')
    iC1.dpid='0001000000000001'
    
    rootswitch = connectToRootNS(net)

    print "*** Connect Hosts to Switches"
    net.addLink(cli, iC3)
    net.addLink(ser, iC1)
    net.addLink(cse, iC3)
    
    
    print "*** Configure Clients"
    cli.cmd('modprobe 8021q')
    cli.cmd('vconfig add %s-eth1 3001' % cli.name)
    cli.cmd('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli.name))
    cli.cmd('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli.name))
    cli.cmd('ip link set dev %s-eth1 up' % cli.name)
    cli.cmd('ip link set dev %s-eth1.3001 up' %cli.name)
    
    print "*** Configure Servers"	 
    ser.cmd('modprobe 8021q')
    ser.cmd('vconfig add %s-eth1 3001' % ser.name)
    ser.cmd('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser.name))
    ser.cmd('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser.name))
    ser.cmd('ip link set dev %s-eth1 up' % ser.name)
    ser.cmd('ip link set dev %s-eth1.3001 up' % ser.name)
    
    print "*** Configure CacheServers"	 
    cse.cmd('modprobe 8021q')
    cse.cmd('vconfig add %s-eth1 3001' % cse.name)
    cse.cmd('ip addr add 192.168.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse.name))
    cse.cmd('ip addr add 172.16.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse.name))
    cse.cmd('ip link set dev %s-eth1 up' % cse.name)
    cse.cmd('ip link set dev %s-eth1.3001 up' % cse.name)
    
    print "*** Connect Switches To Switches"
    net.addLink(iC3, iC1)
    return net

def MultiSiteMNet():
    "MultiSite Medium Topology Example"
    h = 0
    print "*** MultiSite Medium Topology With 2 Client, 2 Server, 2 CacheServer"
    net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )
    
    cli = []
    ser = []
    cse = []
    
    print "*** Creating Clients"
    for h in range(0,2):
      cli.append(net.addHost('cli%s' % (h+1)))
      
    print "*** Creating Servers"  
    for h in range(0,2):
      ser.append(net.addHost('ser%s' % (h+1)))
    
    print "*** Creating CacheServers"
    for h in range(0,2):
      cse.append(net.addHost('cse%s' % (h+1)))
      
    print "*** Creating Switches"
    iC3 = net.addSwitch('iC3')
    iC3.dpid='0001000000000003'
    iC1 = net.addSwitch('iC1')
    iC1.dpid='0001000000000001'
    eTHZ3 = net.addSwitch('eTHZ3')
    eTHZ3.dpid='0200000000000003'
    eTHZ1 = net.addSwitch('eTHZ1')
    eTHZ1.dpid='0200000000000001'
    iM1 = net.addSwitch('iM1')
    iM1.dpid='01000000000000FF'
    
    rootswitch = connectToRootNS(net)

    print "*** Connect Hosts to Switches"
    net.addLink(cli[0], iC3)
    net.addLink(ser[0], iC1)
    net.addLink(cse[0], iC3)
    net.addLink(cli[1], eTHZ1)
    net.addLink(ser[1], eTHZ3)
    net.addLink(cse[1], eTHZ1)
    
    print "*** Configure Clients"
    for h in range(0,2):
      cli[h].cmd('modprobe 8021q')
      cli[h].cmd('vconfig add %s-eth1 3001' % cli[h].name)
      cli[h].cmd('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
      cli[h].cmd('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
      cli[h].cmd('ip link set dev %s-eth1 up' % cli[h].name)
      cli[h].cmd('ip link set dev %s-eth1.3001 up' % cli[h].name)
    
    print "*** Configure Servers"	 
    for h in range(0,2):
      ser[h].cmd('modprobe 8021q')
      ser[h].cmd('vconfig add %s-eth1 3001' % ser[h].name)
      ser[h].cmd('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
      ser[h].cmd('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
      ser[h].cmd('ip link set dev %s-eth1 up' % ser[h].name)
      ser[h].cmd('ip link set dev %s-eth1.3001 up' % ser[h].name)
    
    print "*** Configure CacheServers"	 
    for h in range(0,2):
      cse[h].cmd('modprobe 8021q')
      cse[h].cmd('vconfig add %s-eth1 3001' % cse[h].name)
      cse[h].cmd('ip addr add 192.168.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse[h].name))
      cse[h].cmd('ip addr add 172.16.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse[h].name))
      cse[h].cmd('ip link set dev %s-eth1 up' % cse[h].name)
      cse[h].cmd('ip link set dev %s-eth1.3001 up' % cse[h].name)
    
    print "*** Connect Switches To Switches"
    net.addLink(iC3, iC1)
    net.addLink(eTHZ1, eTHZ3)
    net.addLink(iC1, iM1)
    net.addLink(eTHZ3, iM1)
    return net

def MultiSiteLNet():
    "MultiSite Large Topology Example"
    h = 0
    print "*** MultiSite Large Topology With 3 Client, 3 Server, 3 CacheServer"
    net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )
    
    cli = []
    ser = []
    cse = []
    
    print "*** Creating Clients"
    for h in range(0,3):
      cli.append(net.addHost('cli%s' % (h+1)))
      
    print "*** Creating Servers"  
    for h in range(0,3):
      ser.append(net.addHost('ser%s' % (h+1)))
    
    print "*** Creating CacheServers"
    for h in range(0,3):
      cse.append(net.addHost('cse%s' % (h+1)))
      
    print "*** Creating Switches"
    iC3 = net.addSwitch('iC3')
    iC3.dpid='0001000000000003'
    iC1 = net.addSwitch('iC1')
    iC1.dpid='0001000000000001'
    eTHZ3 = net.addSwitch('eTHZ3')
    eTHZ3.dpid='0200000000000003'
    eTHZ1 = net.addSwitch('eTHZ1')
    eTHZ1.dpid='0200000000000001'
    cN3 = net.addSwitch('cN3')
    cN3.dpid='0208020800000003'
    cN1 = net.addSwitch('cN1')
    cN1.dpid='0208020800000001'
    iM1 = net.addSwitch('iM1')
    iM1.dpid='01000000000000FF'
    
    rootswitch = connectToRootNS(net)

    print "*** Connect Hosts to Switches"
    net.addLink(cli[0], iC3)
    net.addLink(ser[0], iC1)
    net.addLink(cse[0], iC3)
    net.addLink(cli[1], eTHZ1)
    net.addLink(ser[1], eTHZ3)
    net.addLink(cse[1], eTHZ1)
    net.addLink(cli[2], cN3)
    net.addLink(ser[2], cN1)
    net.addLink(cse[2], cN3)
    
    print "*** Configure Clients"
    for h in range(0,3):
      cli[h].cmd('modprobe 8021q')
      cli[h].cmd('vconfig add %s-eth1 3001' % cli[h].name)
      cli[h].cmd('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
      cli[h].cmd('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
      cli[h].cmd('ip link set dev %s-eth1 up' % cli[h].name)
      cli[h].cmd('ip link set dev %s-eth1.3001 up' % cli[h].name)
    
    print "*** Configure Servers"	 
    for h in range(0,3):
      ser[h].cmd('modprobe 8021q')
      ser[h].cmd('vconfig add %s-eth1 3001' % ser[h].name)
      ser[h].cmd('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
      ser[h].cmd('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
      ser[h].cmd('ip link set dev %s-eth1 up' % ser[h].name)
      ser[h].cmd('ip link set dev %s-eth1.3001 up' % ser[h].name)
    
    print "*** Configure CacheServers"	 
    for h in range(0,3):
      cse[h].cmd('modprobe 8021q')
      cse[h].cmd('vconfig add %s-eth1 3001' % cse[h].name)
      cse[h].cmd('ip addr add 192.168.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse[h].name))
      cse[h].cmd('ip addr add 172.16.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse[h].name))
      cse[h].cmd('ip link set dev %s-eth1 up' % cse[h].name)
      cse[h].cmd('ip link set dev %s-eth1.3001 up' % cse[h].name)
    
    print "*** Connect Switches To Switches"
    net.addLink(iC3, iC1)
    net.addLink(eTHZ1, eTHZ3)
    net.addLink(cN3, cN1)
    net.addLink(iC1, iM1)
    net.addLink(eTHZ3, iM1)
    net.addLink(cN1, iM1)
    return net

def MultiSiteXLNet():
    "MultiSite XLarge Topology Example"
    h = 0
    print "*** MultiSite XLarge Topology With 4 Client, 4 Server, 4 CacheServer"
    net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )
    
    cli = []
    ser = []
    cse = []
    
    print "*** Creating Clients"
    for h in range(0,4):
      cli.append(net.addHost('cli%s' % (h+1)))
      
    print "*** Creating Servers"  
    for h in range(0,4):
      ser.append(net.addHost('ser%s' % (h+1)))
    
    print "*** Creating CacheServers"
    for h in range(0,4):
      cse.append(net.addHost('cse%s' % (h+1)))
      
    print "*** Creating Switches"
    iC3 = net.addSwitch('iC3')
    iC3.dpid='0001000000000003'
    iC1 = net.addSwitch('iC1')
    iC1.dpid='0001000000000001'
    eTHZ3 = net.addSwitch('eTHZ3')
    eTHZ3.dpid='0200000000000003'
    eTHZ1 = net.addSwitch('eTHZ1')
    eTHZ1.dpid='0200000000000001'
    cN3 = net.addSwitch('cN3')
    cN3.dpid='0208020800000003'
    cN1 = net.addSwitch('cN1')
    cN1.dpid='0208020800000001'
    tUB2 = net.addSwitch('tUB2')
    tUB2.dpid ='0000000000000402'
    tUB1 = net.addSwitch('tUB1')
    tUB1.dpid='0000000000000401'
    iM1 = net.addSwitch('iM1')
    iM1.dpid='01000000000000FF'
    
    rootswitch = connectToRootNS(net)

    print "*** Connect Hosts to Switches"
    net.addLink(cli[0], iC3)
    net.addLink(ser[0], iC1)
    net.addLink(cse[0], iC3)
    net.addLink(cli[1], eTHZ1)
    net.addLink(ser[1], eTHZ3)
    net.addLink(cse[1], eTHZ1)
    net.addLink(cli[2], cN3)
    net.addLink(ser[2], cN1)
    net.addLink(cse[2], cN3)
    net.addLink(cli[3], tUB1)
    net.addLink(ser[3], tUB2)
    net.addLink(cse[3], tUB1)
    
    print "*** Configure Clients"
    for h in range(0,4):
      cli[h].cmd('modprobe 8021q')
      cli[h].cmd('vconfig add %s-eth1 3001' % cli[h].name)
      cli[h].cmd('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
      cli[h].cmd('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.3001' %((h+1),cli[h].name))
      cli[h].cmd('ip link set dev %s-eth1 up' % cli[h].name)
      cli[h].cmd('ip link set dev %s-eth1.3001 up' % cli[h].name)
    
    print "*** Configure Servers"	 
    for h in range(0,4):
      ser[h].cmd('modprobe 8021q')
      ser[h].cmd('vconfig add %s-eth1 3001' % ser[h].name)
      ser[h].cmd('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
      ser[h].cmd('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.3001' %((h+1),ser[h].name))
      ser[h].cmd('ip link set dev %s-eth1 up' % ser[h].name)
      ser[h].cmd('ip link set dev %s-eth1.3001 up' % ser[h].name)
    
    print "*** Configure CacheServers"	 
    for h in range(0,4):
      cse[h].cmd('modprobe 8021q')
      cse[h].cmd('vconfig add %s-eth1 3001' % cse[h].name)
      cse[h].cmd('ip addr add 192.168.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse[h].name))      
      cse[h].cmd('ip addr add 172.16.128.%s/16 brd + dev %s-eth1.3001' %((h+1),cse[h].name))
      cse[h].cmd('ip link set dev %s-eth1 up' % cse[h].name)
      cse[h].cmd('ip link set dev %s-eth1.3001 up' % cse[h].name)
    
    print "*** Connect Switches To Switches"
    net.addLink(iC3, iC1)
    net.addLink(eTHZ1, eTHZ3)
    net.addLink(cN3, cN1)
    net.addLink(tUB1,tUB2)
    net.addLink(iC1, iM1)
    net.addLink(eTHZ3, iM1)
    net.addLink(cN1, iM1)
    net.addLink(tUB2, iM1)
    return net
    
def copytree(src, dst, symlinks=False, ignore=None):
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)
   
def client_config(name, index):
#  print "Index:", index
  conf = open("./" + name + "/conet.conf", 'w')
  conf.write("if_name = " + name + "-eth1.3001")
  conf.close
  shutil.copy("./install/client-ccnd", "./" + name + "/" + name + "-ccnd")
  copytree("./install/client/", "./" + name + "/")

def server_config(name, index):
#  print "Index:", index
  conf = open("./" + name + "/conet.conf", 'w')
  conf.write("if_name = " + name + "-eth1.3001")
  conf.close
  shutil.copy("./install/server-ccnd", "./" + name + "/" + name + "-ccnd")
  copytree("./install/server/", "./" + name + "/")
  os.mkdir("./" + name + "/repo")
  
def cserver_config(name, index):
#  print "Index:", index
  os.mkdir("./" + name + "/cache")
  copytree("./install/cache/", "./" + name + "/cache/")
  shutil.copy("./install/start", "./" + name + "/")
  shutil.copy("./install/cache/conet.conf", "./" + name + "/")
  conf = open("./" + name + "/conet.conf", 'a')
  conf.write("if_name = " + name + "-eth1.3001\n")
  conf.write("cache_server_ip = 192.168.128.%s" % index)
  os.mkdir("./" + name + "/files")
  os.mkdir("./" + name + "/files/chunks")
  
def host_snmp_config( name ):
  if not os.path.exists("/tmp"):
    os.mkdir("/tmp",ignore_if_existing=True)
  os.mkdir("/tmp/" + name)
  os.mkdir("/tmp/" + name + "/snmp")
  copytree("./install/snmp","/tmp/" + name + "/snmp")
 
def clean_environment( name ):
  shutil.rmtree("/tmp/" + name, ignore_errors=True)
  shutil.rmtree("./" + name, ignore_errors=True)
	
def init_net(net, topo, toCompile = '1'):
    "Init Function"
    print "*** Init"
    if toCompile == '1':
      print "*** Compiling Conet Client, Conet Server and CacheServer"
      subprocess.call(["sh", "../build_all"], stdout=None, stderr=None)
    i = 0
    j = 0
    modulo = len(net.hosts)/3
    "Kill Previous Service If Active"
    subprocess.call(["killall", "snmpd"], stdout=None, stderr=None)
    for host in net.hosts:
      print "Generating Environment For", host.name
      os.mkdir("./" + host.name)
      if 'cli' in (host.name):
	client_config(host.name, i+1)
      elif 'ser' in (host.name):
	server_config(host.name, i+1)
      elif 'cse' in (host.name):
	cserver_config(host.name, i+1)
      host_snmp_config(host.name)
      host.cmd('/usr/sbin/snmpd -Lsd -Lf /dev/null -u snmp -g snmp -I -smux -p /var/run/'
      + host.name + '-snmp.pid -c /tmp/' + host.name + '/snmp/snmpd.conf -C &')
      print "***", host.name, "is running snmpd on " + host.name + "-eth0 10.0.0.%s" % (j+1)
      i = (i + 1)%modulo
      j = j + 1
    net.start()
    print "*** Type 'exit' or control-D to shut down network"
    CLI( net )
    net.stop()
    subprocess.call(["sudo", "mn", "-c"], stdout=None, stderr=None)
    for host in net.hosts:
      print "Cleaning Environment For", host.name
      clean_environment(host.name)
      if 'cse' not in (host.name):
	print "Killing ", host.name + "-ccnd"
	subprocess.call(["sudo", "killall", host.name + "-ccnd"], stdout=None, stderr=None)
    subprocess.call(["killall", "snmpd"], stdout=None, stderr=None)
    
if __name__ == '__main__':
    lg.setLogLevel( 'info')
    if len( sys.argv ) > 2:
      if sys.argv[1] == 'fattree':
	net = FatTree()
      elif sys.argv[1] == 'i2cat':
	net = i2CatNet()
	#init_net(net)
      elif sys.argv[1] == 'multisitem':
	net = MultiSiteMNet()
	#init_net(net)
      elif sys.argv[1] == 'multisitel':
	net = MultiSiteLNet()
	#init_net(net)
      elif sys.argv[1] == 'multisitexl':
	net = MultiSiteXLNet()
	#init_net(net)
      else:
	print "Unknow Topology"
	sys.exit(0)
      init_net(net, sys.argv[1], sys.argv[2])
    else:
      print "You need to insert topo-name (for example fattree)" 
  
   
    




#!/usr/bin/python

# Create one topology, you can chose your topology with cmd line args
# In this file we provide 5 examples of slice:
# 1) FatTree
# 2) Siracusano' s Slice
# 3) Rashed' s Slice
# 4) Ventre' s Slice
# 5) MultiSite XLarge (it contains the resources of TUB's Island and is built on top of Ventre's Slice)
# You can use only one slice at time,
# You can run the experiment with the command: sudo ./start_mininet.py name_in_file compile_option (0/1) debug_mode (0/1)
# ps: for name_in_file sees the name near topos

import sys
import subprocess
import time
import shutil
import os
import getopt

from mininet.net import Mininet
from mininet.cli import CLI
from mininet.log import lg
from mininet.node import RemoteController, Node, OVSKernelSwitch # We use OVSKernelSwitch
from mininet.link import Link


allowed_toponame = ['i2cat', 'multisitem', 'multisitel', 'multisitexl', 'fattree']
TOPO=''
DEBUG=0
toCOMPILE=0
COLUMN = 3
portsToBeMonitored = []

class portToBeMonitored: # This object contains the data for the monitoring; 
			 # In our conventions the src port is the one to be monitored;
     	def __init__(self, dpid, src, dst):
        	self.dpid = dpid
		port = intfToport(src)
		if port < 10:
			self.port = '0' + str(port)
		else:        
			self.port = str(port)
		self.source = str(src)
		self.destination = str(dst)

	def serializeToBeMonitored(self):
		return self.dpid + "|" + self.port + "|" + self.source.upper() + " Connected To " + self.destination.upper() + "\n"
		

def intfToport(intf):
	intf = str(intf)
	a = intf.split('-')
	if len(a) > 2:
		print "*** WARNING BAD NAME FOR INTF - EXIT"
		sys.exit(-1)
	return int(a[1][3:])

def fixSwitchIntf(swi):
  for i in range(0, len(swi)):
    for obj in swi[i].nameToIntf:
      if 'lo' not in obj:
	fixNetworkManager(obj)	
    fixNetworkManager(swi[i])    
  root = Node( 'root', inNamespace=False )
  print "Restarting Network Manager"
  time.sleep(10)
  root.cmd('service network-manager restart')
  time.sleep(2)
  
def fixNetworkManager(intf):
  cfile = '/etc/network/interfaces'
  #line1 = '\nauto %s\n' % intf
  line1 = 'iface %s inet manual\n' % intf
  #line2 = '\tiface %s inet static\n' % intf
  config = open( cfile ).read()
  if ( line1 ) not in config:
    print '*** Adding', line1.strip(), 'to', cfile
    #print '*** Adding', line2.strip(), 'to', cfile
    with open( cfile, 'a' ) as f:
      f.write( line1 )
      #f.write( line2 )
    f.close();


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
	print "*** Added Interface", str(intf), "Connected To Root-NameSpace"
	fixNetworkManager(str(intf))
	for host in net.hosts:
	    net.addLink(host,rootswitch)
	# Add routes from root ns to hosts
	for route in routes:
         root.cmd( 'route add -net ' + route + ' dev ' + str( intf ) )
	root.cmd('service network-manager restart')
	return rootswitch

def node_config(node, index):
	if DEBUG == 1:
	  node.cmdPrint('modprobe 8021q')
	  node.cmdPrint('vconfig add %s-eth1 301' % node.name)
	  if "cli" in node.name:
		  node.cmdPrint('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.301' %(index,node.name))
		  node.cmdPrint('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.301' %(index,node.name))
	  elif "ser" in node.name:
		  node.cmdPrint('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.301' %(index,node.name))
		  node.cmdPrint('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.301' %(index,node.name))
	  elif "cse" in node.name:
		  node.cmdPrint('ip addr add 192.168.128.%s/16 brd + dev %s-eth1.301' %(index,node.name))
		  node.cmdPrint('ip addr add 172.16.128.%s/16 brd + dev %s-eth1.301' %(index,node.name))
	  node.cmdPrint('ip link set %s-eth1 up' % node.name)
	  node.cmdPrint('ip link set %s-eth1.301 up' % node.name)
	else :
	  node.cmd('modprobe 8021q')
	  node.cmd('vconfig add %s-eth1 301' % node.name)
	  if "cli" in node.name:
		  node.cmd('ip addr add 192.168.0.%s/16 brd + dev %s-eth1.301' %(index,node.name))
		  node.cmd('ip addr add 172.16.0.%s/16 brd + dev %s-eth1.301' %(index,node.name))
	  elif "ser" in node.name:
		  node.cmd('ip addr add 192.168.64.%s/16 brd + dev %s-eth1.301' %(index,node.name))
		  node.cmd('ip addr add 172.16.64.%s/16 brd + dev %s-eth1.301' %(index,node.name))
	  elif "cse" in node.name:
		  node.cmd('ip addr add 192.168.128.%s/16 brd + dev %s-eth1.301' %(index,node.name))
		  node.cmd('ip addr add 172.16.128.%s/16 brd + dev %s-eth1.301' %(index,node.name))
	  node.cmd('ip link set %s-eth1 up' % node.name)
	  node.cmd('ip link set %s-eth1.301 up' % node.name)

def Mesh(switches=4):
	"Create A Mesh Topo"
	clients = servers = cservers = switches
	print "*** Mesh With", switches, "Switches,", clients, "Client,", servers, "Servers,", cservers, "CacheServers"
	"Create Switches And Hosts"
        cli = []
        ser = []
        cse = []
        swi = []
	links = [] # Links to be monitored
	net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )
	i = 0
	h = 0
	print "*** Creating Clients"
	for h in range(0, clients):
		 cli.append(net.addHost('cli%s' % (h+1)))
	print "*** Creating Servers"
	for h in range(0, servers):
		 ser.append(net.addHost('ser%s' % (h+1)))
	print "*** Creating CacheServers"
	for h in range(0, cservers):
		 cse.append(net.addHost('cse%s' % (h+1)))
        print "*** Creating Switches"
	root = connectToRootNS(net)
	# When you create a new topology you have to save in some way the link and sw that you want to be monitored
	for i in range(switches):
		sw = (net.addSwitch('s%s_c' % (i+1)))
		print "Connect", cli[i], "To", sw
		print "Connect", ser[i], "To", sw
		print "Connect", cse[i], "To", sw
		net.addLink(cli[i], sw)
	    	net.addLink(ser[i], sw)
	    	net.addLink(cse[i], sw)
		for rhs in swi:
			if i == (switches-1):
				links.append(net.addLink(sw,rhs))
			else:
                		net.addLink(sw, rhs)
			print "Connect", sw, "To", rhs   
		swi.append(sw)	
	print "*** Configure Clients"
	for h in range(0,clients):
		node_config(cli[h],(h+1))

	print "*** Configure Servers"	 
	for h in range(0,servers):
		node_config(ser[h],(h+1))

	print "*** Configure CacheServers"	 
	for h in range(0,cservers):
		node_config(cse[h],(h+1))
	print "*** Prepare Link To Be Monitored"
	
	for link in links:	
		portsToBeMonitored.append(portToBeMonitored( 	swi[len(swi)-1].dpid, # Switch To Be Monitored
								str(link.intf1), # Intf To Be Monitored
								str(link.intf2))) # Connected To
	swi.append(root)
	fixSwitchIntf(swi)
	return net
         
def FatTree():                                                                
        "Create FatTree topo."
	print "*** FatTree With 4 Client, 4 Server, 4 CacheServer"
        # Create hosts
        cli = []
        ser = []
        cse = []
        swi = []
	links = [] # Links to be monitored
       	
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
		if s < 4:
                	swi.append(net.addSwitch('s%s_c' % (s+1)))
		else:			
                	swi.append(net.addSwitch('s%s' % (s+1)))
                #swi[s].dpid = '00000000000000%s' % (s+1) # Change this line with more sws, for our scope is ok
	root = connectToRootNS(net)
        print "*** Connect Hosts to Switches" 
        for h in range(0,4):
	    net.addLink(cli[h], swi[h])
	    net.addLink(ser[h], swi[h])
	    net.addLink(cse[h], swi[h])
	
	print "*** Configure Clients"
	for h in range(0,4):
		node_config(cli[h],(h+1))

	print "*** Configure Servers"	 
	for h in range(0,4):
		node_config(ser[h],(h+1))

	print "*** Configure CacheServers"	 
	for h in range(0,4):
		node_config(cse[h],(h+1))

        print "*** Connect Switches To Switches"
        net.addLink(swi[0], swi[4])
        net.addLink(swi[1], swi[4])
        net.addLink(swi[2], swi[5])
	# When you create a new topology you have to save in some way the link and sw that you want to be monitored
        links.append(net.addLink(swi[3], swi[5]))
        net.addLink(swi[4], swi[6])
        net.addLink(swi[5], swi[6])

	for link in links:	
		portsToBeMonitored.append(portToBeMonitored( 	swi[3].dpid, # Switch To Be Monitored
								str(link.intf1), # Intf To Be Monitored
								str(link.intf2))) # Connected To
	swi.append(root)
	fixSwitchIntf(swi)
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
	iC3 = net.addSwitch('iC3_c')
	iC3.dpid='0010000000000003'
	iC1 = net.addSwitch('iC1')
	iC1.dpid='0010000000000001'

	root = connectToRootNS(net)

	print "*** Connect Hosts to Switches"
	net.addLink(cli, iC3)
	net.addLink(ser, iC1)
	net.addLink(cse, iC3)

	print "*** Configure Clients"
	node_config(cli,1)

	print "*** Configure Servers"	 
	node_config(ser,1)

	print "*** Configure CacheServers"	 
	node_config(cse,1)

	print "*** Connect Switches To Switches"
	o = net.addLink(iC3, iC1)

	swi = [iC3,iC1]
	swi.append(root)
	fixSwitchIntf(swi)
	return net

def MultiSiteMNet():
	"MultiSite Medium Topology Example"
	h = 0
	print "*** MultiSite Medium Topology With 2 Client, 2 Server, 2 CacheServer"
	net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )

	cli = []
	ser = []
	cse = []
	links = [] # Links to be monitored

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
	iC3 = net.addSwitch('iC3_c')
	iC3.dpid='0010000000000003'
	iC1 = net.addSwitch('iC1')
	iC1.dpid='0010000000000001'
	eTHZ3 = net.addSwitch('eTHZ3')
	eTHZ3.dpid='0200000000000003'
	eTHZ1 = net.addSwitch('eTHZ1_c')
	eTHZ1.dpid='0200000000000001'
	iM1 = net.addSwitch('iM1')
	iM1.dpid='01000000000000FF'

	root = connectToRootNS(net)

	print "*** Connect Hosts to Switches"
	net.addLink(cli[0], iC3)
	net.addLink(ser[0], iC1)
	net.addLink(cse[0], iC3)
	net.addLink(cli[1], eTHZ1)
	net.addLink(ser[1], eTHZ3)
	net.addLink(cse[1], eTHZ1)

	print "*** Configure Clients"
	for h in range(0,2):
		node_config(cli[h],(h+1))

	print "*** Configure Servers"	 
	for h in range(0,2):
		node_config(ser[h],(h+1))

	print "*** Configure CacheServers"	 
	for h in range(0,2):
		node_config(cse[h],(h+1))

	print "*** Connect Switches To Switches"
	net.addLink(iC3, iC1)
	net.addLink(eTHZ1, eTHZ3)
	# When you create a new topology you have to save in some way the link and sw that you want to be monitored
	links.append(net.addLink(iC1, iM1))
	links.append(net.addLink(eTHZ3, iM1))
	for link in links:	
		portsToBeMonitored.append(portToBeMonitored( 	iM1.dpid, # Switch To Be Monitored
								str(link.intf2), # Intf To Be Monitored
								str(link.intf1))) # Connected To
	swi = [iC3,iC1,eTHZ1,eTHZ3,iM1]
	swi.append(root)
	fixSwitchIntf(swi)
	return net

def MultiSiteLNet():
	"MultiSite Large Topology Example"
	h = 0
	print "*** MultiSite Large Topology With 3 Client, 3 Server, 3 CacheServer"
	net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )

	cli = []
	ser = []
	cse = []
	swi = []
	links = [] # Links to be monitored

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
	iC3 = net.addSwitch('iC3_c')
	iC3.dpid='0010000000000003'
	iC1 = net.addSwitch('iC1')
	iC1.dpid='0010000000000001'
	eTHZ3 = net.addSwitch('eTHZ3')
	eTHZ3.dpid='0200000000000003'
	eTHZ1 = net.addSwitch('eTHZ1_c')
	eTHZ1.dpid='0200000000000001'
	cN3 = net.addSwitch('cN3_c')
	cN3.dpid='0208020800000003'
	cN1 = net.addSwitch('cN1')
	cN1.dpid='0208020800000001'
	iM1 = net.addSwitch('iM1')
	iM1.dpid='01000000000000FF'

	root = connectToRootNS(net)

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
		node_config(cli[h],(h+1))

	print "*** Configure Servers"	 
	for h in range(0,3):
		node_config(ser[h],(h+1))

	print "*** Configure CacheServers"	 
	for h in range(0,3):
		node_config(cse[h],(h+1))

	print "*** Connect Switches To Switches"
	net.addLink(iC3, iC1)
	net.addLink(eTHZ1, eTHZ3)
	net.addLink(cN3, cN1)
	# When you create a new topology you have to save in some way the link and sw that you want to be monitored
	links.append(net.addLink(iC1, iM1))
	links.append(net.addLink(eTHZ3, iM1))
	links.append(net.addLink(cN1, iM1))
	for link in links:	
		portsToBeMonitored.append(portToBeMonitored( 	iM1.dpid, # Switch To Be Monitored
								str(link.intf2), # Intf To Be Monitored
								str(link.intf1))) # Connected To
	swi = [iC3,iC1,eTHZ1,eTHZ3,cN3,cN1,iM1]
	swi.append(root)
	fixSwitchIntf(swi)
	return net

def MultiSiteXLNet():
	"MultiSite XLarge Topology Example"
	h = 0
	print "*** MultiSite XLarge Topology With 4 Client, 4 Server, 4 CacheServer"
	net = Mininet( controller=RemoteController, switch=OVSKernelSwitch, build=False )    

	cli = []
	ser = []
	cse = []
	swi = []
	links = [] # Links to be monitored

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
	iC3 = net.addSwitch('iC3_c')
	iC3.dpid='0010000000000003'
	iC1 = net.addSwitch('iC1')
	iC1.dpid='0010000000000001'
	eTHZ3 = net.addSwitch('eTHZ3')
	eTHZ3.dpid='0200000000000003'
	eTHZ1 = net.addSwitch('eTHZ1_c')
	eTHZ1.dpid='0200000000000001'
	cN3 = net.addSwitch('cN3_c')
	cN3.dpid='0208020800000003'
	cN1 = net.addSwitch('cN1')
	cN1.dpid='0208020800000001'
	tUB2 = net.addSwitch('tUB2')
	tUB2.dpid ='0000000000000402'
	tUB1 = net.addSwitch('tUB1_c')
	tUB1.dpid='0000000000000401'
	iM1 = net.addSwitch('iM1')
	iM1.dpid='01000000000000FF'

	root = connectToRootNS(net)

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
		node_config(cli[h],(h+1))

	print "*** Configure Servers"	 
	for h in range(0,4):
		node_config(ser[h],(h+1))

	print "*** Configure CacheServers"	 
	for h in range(0,4):
		node_config(cse[h],(h+1))

	print "*** Connect Switches To Switches"
	net.addLink(iC3, iC1)
	net.addLink(eTHZ1, eTHZ3)
	net.addLink(cN3, cN1)
	net.addLink(tUB1,tUB2)
	# When you create a new topology you have to save in some way the link and sw that you want to be monitored
	links.append(net.addLink(iC1, iM1))
	links.append(net.addLink(eTHZ3, iM1))
	links.append(net.addLink(cN1, iM1))
	links.append(net.addLink(tUB2, iM1))
	
	for link in links:	
		portsToBeMonitored.append(portToBeMonitored( 	iM1.dpid, # Switch To Be Monitored
								str(link.intf2), # Intf To Be Monitored
								str(link.intf1))) # Connected To
	swi = [iC3,iC1,eTHZ1,eTHZ3,cN3,cN1,tUB1,tUB2,iM1]
	swi.append(root)
	fixSwitchIntf(swi)
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
  conf.write("if_name = " + name + "-eth1.301")
  conf.close
  shutil.copy("./install/client-ccnd", "./" + name + "/" + name + "-ccnd")
  copytree("./install/client/", "./" + name + "/")
  
def server_config(name, index):
#  print "Index:", index
  conf = open("./" + name + "/conet.conf", 'w')
  conf.write("if_name = " + name + "-eth1.301")
  conf.close
  shutil.copy("./install/server-ccnd", "./" + name + "/" + name + "-ccnd")
  copytree("./install/server/", "./" + name + "/")
  os.mkdir("./" + name + "/repo")
  
def cserver_config(name, index):
#  print "Index:", index
  os.mkdir("./" + name + "/cache")
  copytree("./install/cache/", "./" + name + "/cache/")
  os.rename("./" + name + "/cache/listener.o", "./" + name + "/cache/" + name + "-cache")
  shutil.copy("./install/start", "./" + name + "/")
  shutil.copy("./install/cache/conet.conf", "./" + name + "/")
  conf = open("./" + name + "/conet.conf", 'a')
  conf.write("if_name = " + name + "-eth1.301\n")
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

def host_mrtg_config(name, index, index2):
  conf = open("/home/mrtg/cfg/mrtg-temp.cfg","a")
  if "cli" in name:
	conf.write("##################\n")
	conf.write("#  System: %s  #\n" %(name))
	conf.write("##################\n")
	conf.write("Target[%s_192.168.0.%s]: /192.168.0.%s:public@10.0.0.%s:\n" %(name, index, index, index2))
	conf.write("SetEnv[%s_192.168.0.%s]: MRTG_INT_IP=\"192.168.0.%s\" MRTG_INT_DESCR=\"%s-eth1.301\"\n" %(name, index, index, name))
	conf.write("Directory[%s_192.168.0.%s]: %s_%s\n" %(name, index, name, TOPO))
	conf.write("MaxBytes[%s_192.168.0.%s]: 125000000\n" %(name, index))
	conf.write("Title[%s_192.168.0.%s]: Traffic Analysis For %s -- 192.168.0.%s\n" %(name, index, name, index))
	conf.write("PageTop[%s_192.168.0.%s]: <h1>Traffic Analysis For %s -- 192.168.0.%s</h1>\n" %(name, index, name, index))
	conf.write("\t<div id=\"sysdetails\">\n")
	conf.write("\t\t<table>\n")
        conf.write("\t\t\t<tr><td>System:</td><td> %s In Topology %s</td></tr>\n" %(name, TOPO))
        conf.write("\t\t\t<tr><td>ifName:</td><td>%s-eth1.301</td></tr>\n" %(name))
	conf.write("\t\t\t<tr><td>Max Speed:</td><td>1000.0 Mbits/s</td></tr>\n")
	conf.write("\t\t\t<tr><td>IP:</td><td>192.168.0.%s</td></tr>\n" %(index))
	conf.write("\t\t</table>\n")
	conf.write("\t</div>\n")
  elif "ser" in name:
	conf.write("##################\n")
	conf.write("#  System: %s  #\n" %(name))
	conf.write("##################\n")
	conf.write("Target[%s_192.168.64.%s]: /192.168.64.%s:public@10.0.0.%s:\n" %(name, index, index, index2))
	conf.write("SetEnv[%s_192.168.64.%s]: MRTG_INT_IP=\"192.168.64.%s\" MRTG_INT_DESCR=\"%s-eth1.301\"\n" %(name, index, index, name))
	conf.write("Directory[%s_192.168.64.%s]: %s_%s\n" %(name, index, name, TOPO))
	conf.write("MaxBytes[%s_192.168.64.%s]: 125000000\n" %(name, index))
	conf.write("Title[%s_192.168.64.%s]: Traffic Analysis For %s -- 192.168.64.%s\n" %(name, index, name, index))
	conf.write("PageTop[%s_192.168.64.%s]: <h1>Traffic Analysis For %s -- 192.168.64.%s</h1>\n" %(name, index, name, index))
	conf.write("\t<div id=\"sysdetails\">\n")
	conf.write("\t\t<table>\n")
        conf.write("\t\t\t<tr><td>System:</td><td> %s In Topology %s</td></tr>\n" %(name, TOPO))
        conf.write("\t\t\t<tr><td>ifName:</td><td>%s-eth1.301</td></tr>\n" %(name))
	conf.write("\t\t\t<tr><td>Max Speed:</td><td>1000.0 Mbits/s</td></tr>\n")
	conf.write("\t\t\t<tr><td>IP:</td><td>192.168.64.%s</td></tr>\n" %(index))
	conf.write("\t\t</table>\n")
	conf.write("\t</div>\n")
  elif "cse" in name:
	conf.write("##################\n")
	conf.write("#  System: %s  #\n" %(name))
	conf.write("##################\n")
	conf.write("Target[%s_192.168.128.%s]: /192.168.128.%s:public@10.0.0.%s:\n" %(name, index, index, index2))
	conf.write("SetEnv[%s_192.168.128.%s]: MRTG_INT_IP=\"192.168.128.%s\" MRTG_INT_DESCR=\"%s-eth1.301\"\n" %(name, index, index, name))
	conf.write("Directory[%s_192.168.128.%s]: %s_%s\n" %(name, index, name, TOPO))
	conf.write("MaxBytes[%s_192.168.128.%s]: 125000000\n" %(name, index))
	conf.write("Title[%s_192.168.128.%s]: Traffic Analysis For %s -- 192.168.128.%s\n" %(name, index, name, index))
	conf.write("PageTop[%s_192.168.128.%s]: <h1>Traffic Analysis For %s -- 192.168.128.%s</h1>\n" %(name, index, name, index))
	conf.write("\t<div id=\"sysdetails\">\n")
	conf.write("\t\t<table>\n")
        conf.write("\t\t\t<tr><td>System:</td><td> %s In Topology %s</td></tr>\n" %(name, TOPO))
        conf.write("\t\t\t<tr><td>ifName:</td><td>%s-eth1.301</td></tr>\n" %(name))
	conf.write("\t\t\t<tr><td>Max Speed:</td><td>1000.0 Mbits/s</td></tr>\n")
	conf.write("\t\t\t<tr><td>IP:</td><td>192.168.128.%s</td></tr>\n" %(index))
	conf.write("\t\t</table>\n")
	conf.write("\t</div>\n")	
  conf.close()

def conf_portsToBeMonitored():
	if len(portsToBeMonitored) == 0:
		print "*** WARNING NO LINK WILL BE MONITORED"
	else:
		monitoring_conf = open("/opt/lampp/cgi-bin/monitor.conf","w")
		for port in portsToBeMonitored:
			monitoring_conf.write(port.serializeToBeMonitored())
		monitoring_conf.close()
	

	
def init_net(net):
    "Init Function"
    print "*** Init Environment"
    if toCOMPILE == 1:
      print "*** Compiling Conet Client, Conet Server and CacheServer"
      subprocess.call(["sh", "../build_all"], stdout=None, stderr=None)
    i = 0
    j = 0
    modulo = len(net.hosts)/3
    print "*** Kill Previous Service SNMP/SSH If Active"
    subprocess.call(["killall", "snmpd"], stdout=None, stderr=None)
    subprocess.call(["killall", "sshd"], stdout=None, stderr=None)
    subprocess.call(["killall", "mrtg"], stdout=None, stderr=None)
    shutil.copy("./install/mrtg/mrtg-temp.cfg", "/home/mrtg/cfg/")
    mrtg_conf = open("/etc/mrtg-rrd.conf","w")
    mrtg_conf.write("/home/mrtg/cfg/mrtg-temp.cfg\n")
    mrtg_conf.close()
    for host in net.hosts:
      clean_environment(host.name)
      print "*** Generating Environment For", host.name
      os.mkdir("./" + host.name)
      if 'cli' in (host.name):
	client_config(host.name, i+1)
      elif 'ser' in (host.name):
	server_config(host.name, i+1)
      elif 'cse' in (host.name):
	cserver_config(host.name, i+1)
      host_mrtg_config(host.name, (i+1), (j+1))
      i = (i + 1)%modulo
      j = j + 1
    subprocess.call(["sudo", "env", "LANG=C", "/usr/bin/mrtg", "--user=nobody", "--group=nogroup", "/home/mrtg/cfg/mrtg-temp.cfg"],stdout=None,stderr=None)
    i = 0
    j = 0
    ip = ""
    host_conf = open("/opt/lampp/cgi-bin/host.conf","w")
    host_conf.write("%s\n" %(TOPO))
    host_conf.write("%s\n" %(COLUMN))
    for i in range(0,modulo):
      host_conf.write("%s|192.168.0.%s\n" %(net.hosts[i],i+1))
      host_conf.write("%s|192.168.64.%s\n" %(net.hosts[i+modulo],i+1))
      host_conf.write("%s|192.168.128.%s\n" %(net.hosts[i+(2*modulo)],i+1))
    for host in net.hosts:
      host_snmp_config(host.name)
      host.cmd('/usr/sbin/snmpd -Lsd -Lf /dev/null -u snmp -g snmp -I -smux -p /var/run/'
      + host.name + '-snmp.pid -c /tmp/' + host.name + '/snmp/snmpd.conf -C &')
      host.cmd('/usr/sbin/sshd -D &')
      print "***", host.name, "is running snmpd and sshd on " + host.name + "-eth0 10.0.0.%s" % (j+1)
      j = j + 1
    switch_conf = open("/opt/lampp/cgi-bin/switch.conf","w")
    for sw in net.switches:
	if 'FFFFFFFFFFFFFFFF' not in (sw.dpid):
    		switch_conf.write("%s|%s\n" %(sw.name,sw.dpid))
	if '_c' in sw.name:
		host_conf.write("csw%s|%s\n" %(sw.name[:-2],sw.dpid)) #cswic3|0001000000000001
    switch_conf.close()
    host_conf.close()
    root = Node( 'root', inNamespace=False )
    root.cmd('stop avahi-daemon')
    root.cmd('killall dhclient')
    conf_portsToBeMonitored()
    net.start()
    print "*** Type 'exit' or control-D to shut down network"
    CLI( net )
    net.stop()
    subprocess.call(["sudo", "mn", "-c"], stdout=None, stderr=None)
    for host in net.hosts:
      print "*** Cleaning Environment For", host.name
      clean_environment(host.name)
      if 'cse' not in (host.name):
	print "*** Kill ", host.name + "-ccnd"
	subprocess.call(["sudo", "killall", host.name + "-ccnd"], stdout=None, stderr=None)
    print "Kill Service SNMP/SSH"
    subprocess.call(["killall", "snmpd"], stdout=None, stderr=None)
    subprocess.call(["killall", "sshd"], stdout=None, stderr=None)
    subprocess.call(["killall", "mrtg"], stdout=None, stderr=None) 
    os.remove("/home/mrtg/cfg/mrtg-temp.cfg") 
    root.cmd('service network-manager restart')
    root.cmd('start avahi-daemon')  


def usage():
	print "\n\n############################################### USAGE #######################################################\n"
	print "-t toponame\tIn Order To Choose A Topology: i2cat, multisitem, multisitel, multisitexl, fattree, mesh[x]"
	print "--topo=toponame\tIn Order To Choose A Topology: i2cat, multisitem, multisitel, multisitexl, fattree, mesh[x]\n"
	print "-c\t\tIn Order To Compile CCNX Executable, this option is needed only the first time"
	print "--compile\tIn Order To Compile CCNX Executable, this option is needed only the first time\n"
	print "-v\t\tIn Order To Activate The Script's Verbose Mode"
	print "--verbose\tIn Order To Activate The Script's Verbose Mode\n"
	print "#############################################################################################################\n"

def parse_args(args):
	global TOPO
	global DEBUG
	global toCOMPILE
	switches = 0
	try:
        	opts, args = getopt.getopt(args[1:], "t:cvh", ["topo=", "compile", "verbose", "help"])
		for option in opts:
			if (option[0] == '-t') or (option[0] == '--topo'):
				if option[1] in allowed_toponame:
					TOPO = option[1]
				elif "mesh" in option[1]:
					TOPO = 'mesh'
					firstlines = option[1].split('[')
					if len(firstlines) > 1:
						firstlines = firstlines[1:]
						parameter = firstlines[0].split(']')
						try: 
							switches = int(parameter[0])
		    				except ValueError:
							print "Invalid Parameter For Mesh - Using Default Parameter"
							switches = 4
					else:
						print "Invalid Parameter For Mesh - Using Default Parameter"
						switches = 4
				else:
					raise getopt.GetoptError("Topology Not Allowed")
			if (option[0] == '-c') or (option[0] == '--compile'):
				toCOMPILE = 1
			if (option[0] == '-v') or (option[0] == '--verbose'):
				DEBUG = 1
			if (option[0] == '-h') or (option[0] == '--help'):
				usage()
				sys.exit(0)
    	except getopt.GetoptError as err:
        	print "\n",str(err)
		usage()
        	sys.exit(2)
	return (switches)


if __name__ == '__main__':
	sw = parse_args(sys.argv)	
	if TOPO != '':
		net = None
		lg.setLogLevel('info')
		if DEBUG != 0:
			print "########################################################################"
			print "###                          VERBOSE MODE                            ###"
			print "########################################################################"
		if TOPO == 'fattree':
			net = FatTree()
		elif TOPO == 'i2cat':
			net = i2CatNet()
		elif TOPO == 'multisitem':
			net = MultiSiteMNet()
		elif TOPO == 'multisitel':
			net = MultiSiteLNet()
		elif TOPO == 'multisitexl':
			net = MultiSiteXLNet()
		elif  TOPO == 'mesh':
			TOPO = 'mesh_' + str(sw)
			net = Mesh(sw)
  		else:
			print "\nUnknow Topology"
			usage()
			sys.exit(2)
		init_net(net)
	else: 
		print "\nUnknow Topology"
		usage()
		sys.exit(2)
	

          


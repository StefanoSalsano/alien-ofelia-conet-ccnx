#!/usr/bin/perl -w
#
# mrtg-rrd2.cgi: A script for generating graphs for MRTG statistics,
# modified starting from mrtg-rrd.cgi 
#
# in our configuration, it is stored in /opt/lampp/cgi-bin/mrtg-rrd2.cgi
# wget https://www.dropbox.com/s/nu8nm8ebh3f4l0e/mrtg-rrd2.cgi
#
# simple local test: /usr/bin/perl -w mrtg-rrd2.cgi
# local test with parameters using the shell script
# /opt/lampp/cgi-bin/test-mrtg-rrd2.sh
# wget https://www.dropbox.com/s/334529psoett16t/test-mrtg-rrd2.sh

#
# Loosely modelled after the Rainer.Bawidamann@informatik.uni-ulm.de's
# 14all.cgi
#
#   Copyright (C) 2001 Jan "Yenya" Kasprzak <kas@fi.muni.cz>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 

use strict;

use POSIX qw(strftime);
use Time::Local;

# The %.1f should work on 5.005+. There may be other problems, though.
# I've tested this on 5.8.0 only, so mind the gap!
require 5.005;

# Location of RRDs.pm, if it is not in @INC
# use lib '/usr/lib/perl5/5.00503/i386-linux';
use RRDs;

use vars qw(@config_files @all_config_files %targets $config_time
	%directories $version $imagetype);

#USEFUL FOR DEBUGGING: PRINTS ALL THE CGI ENVIRONMENT VARIABLES
#use CGI qw(:standard);
#use CGI::Carp qw(warningsToBrowser fatalsToBrowser);
#
#print header;
#print start_html("Environment");
#
#foreach my $key (sort(keys(%ENV))) {
#    print "$key = $ENV{$key}<br>\n";
#}
#
#print end_html;
#return;


my $running_where = "local_vm" ;    # use this option if the mrtg-rrd2.cgi is running on a local VM in the 
                                    # experimenter's local host (in this case we can control both a remote experiment
                                    # running in the OFELIA testbed and a local experiment running on mininet)
# my $running_where = "remote_vm" ; # use this option if the mrtg-rrd2.cgi is running on a VM in the OFELIA testbed


#my $global_conf = "i2cat";
#my $global_conf = "trento";
#my $global_conf = "multi12cat";
my $global_conf = "local"; #it reads file host.conf stored in the cgi-bin folder


###########################################################
# reading URL parameters
my ($buffer, @pairs, $pair, $name, $value, %FORM);
# Read in text
$ENV{'REQUEST_METHOD'} =~ tr/a-z/A-Z/;
if ($ENV{'REQUEST_METHOD'} eq "GET") {
	$buffer = $ENV{'QUERY_STRING'};
}
# Split information into name/value pairs
@pairs = split(/&/, $buffer);
foreach $pair (@pairs) {
	($name, $value) = split(/=/, $pair);
	$value =~ tr/+/ /;
	$value =~ s/%(..)/pack("C", hex($1))/eg;
	$FORM{$name} = $value;
}

# $target defines the target testbed 
# $target can be trento (for trento multisite) i2cat (for barcellona single site)
# multi12cat (barcellona multi site) local (mininet) 
# "" (in this case it takes the default value of $global_conf
my $target = $FORM{target};

# end reading URL parameters
###########################################################

# redefines $global_conf from target URL parameter if present
if (! ($target eq "") ) {
	$global_conf = $target;
}

my $web_page_header = "Evaluation of ICN Caching";
$web_page_header = $web_page_header. " (OFELIA slice i2cat)" if ($global_conf eq "i2cat");
$web_page_header = $web_page_header. " (OFELIA slice multisite)" if ($global_conf eq "trento");
$web_page_header = $web_page_header. " (OFELIA slice multisite small)" if ($global_conf eq "multi12cat");
$web_page_header = $web_page_header. " (local mininet experiment)" if ($global_conf eq "local");


my $num_of_columns = 0;
$num_of_columns = 2 if ($global_conf eq "i2cat");
$num_of_columns = 4 if ($global_conf eq "trento");
$num_of_columns = 3 if ($global_conf eq "multi12cat");
$num_of_columns = 3 if ($global_conf eq "local");


#the address of the management node
my $host_ip="";  

if ($running_where eq "remote_vm") {
	$host_ip = "10.216.12.85" if ($global_conf eq "i2cat" || $global_conf eq "multi12cat"); 
	$host_ip = "10.216.33.91" if ($global_conf eq "trento");  
	
	#NB $running_where = "remote_vm" and $global_conf eq "local" makes no sense
	$host_ip = "127.0.0.1" if ($global_conf eq "local");      
	
} elsif ($running_where eq "local_vm") {
	$host_ip = "127.0.0.1";
}


my @cell_heading = ();
my @cell_png = ();
if ($global_conf eq "i2cat") {
	my $append_query = "?&target=i2cat";
	push (@cell_heading, "ICN client 192.168.0.1"); 
	push (@cell_png, "icnclient_/icnclient_eth1-hour.png$append_query");
	push (@cell_heading, "ICN server 192.168.64.1");
	push (@cell_png, "icnserver_/icnserver_eth2-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.1");
	push (@cell_png, "cacheserver_/cacheserver_eth2-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.2");
	push (@cell_png, "cacheserver_/cacheserver2_eth1-hour.png$append_query");

	push (@cell_heading, "Cached items i2C-01");
	push (@cell_png, "controller/i2cat/00.10.00.00.00.00.00.01cached_items.png$append_query");

	push (@cell_heading, "Cached items i2C-03");
	push (@cell_png, "controller/i2cat/00.10.00.00.00.00.00.03cached_items.png$append_query");

	push (@cell_heading, "Number of cached items");
	push (@cell_png, "controller/cached_items.png$append_query");
}

if ($global_conf eq "trento") {
	my $append_query = "?&target=trento";
	push (@cell_heading, "ICN client 192.168.0.1 (CreateNet)"); 
	push (@cell_png, "vm3client1_/vm3client1_eth1-hour.png$append_query");
	push (@cell_heading, "ICN server 192.168.64.1 (CreateNet)");
	push (@cell_png, "vm1server1_/vm1server1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.1 (CreateNet)");
	push (@cell_png, "vm3cache1_/vm3cache1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.4 (CreateNet)");
	push (@cell_png, "vm1cache1_/vm1cache1_eth1-hour.png$append_query");

	push (@cell_heading, "ICN client 192.168.0.2 (ETHZ)"); 
	push (@cell_png, "et2client1_/et2client1_eth1-hour.png$append_query");
	push (@cell_heading, "ICN server 192.168.64.2 (ETHZ)");
	push (@cell_png, "et3server1_/et3server1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.2 (ETHZ)");
	push (@cell_png, "et2cache1_/et2cache1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.5 (ETHZ)");
	push (@cell_png, "et3cache1_/et3cache1_eth1-hour.png$append_query");

	push (@cell_heading, "ICN client 192.168.0.3 (i2cat)"); 
#	push (@cell_png, "verclient1_/verclient1_eth1-hour.png$append_query");
	push (@cell_png, "rodserver1_/rodserver1_eth1-hour.png$append_query");

	push (@cell_heading, "ICN server 192.168.64.3 (i2cat)");
#	push (@cell_png, "rodserver1_/rodserver1_eth1-hour.png$append_query");
	push (@cell_png, "verclient1_/verclient1_eth1-hour.png$append_query");

	push (@cell_heading, "Cache server 192.168.128.3 (i2cat)");
	push (@cell_png, "vercache1_/vercache1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.6 (i2cat)");
	push (@cell_png, "rodcache1_/rodcache1_eth1-hour.png$append_query");

	
	push (@cell_heading, "Cached items VM3 CN-03");
	push (@cell_png, "controller/multisite/02.08.02.08.00.00.00.03cached_items.png$append_query");

#	push (@cell_heading, "???Cached items VM1?? CN-01");
#	push (@cell_png, "controller/multisite/02.08.02.08.00.00.00.01cached_items.png$append_query");
	
	push (@cell_heading, "Cached items ETHZ2 ETH-01");
	push (@cell_png, "controller/multisite/02.00.00.00.00.00.00.01cached_items.png$append_query");

#	push (@cell_heading, "???Cached items ETHZ2 ETH-03");
#	push (@cell_png, "controller/multisite/02.00.00.00.00.00.00.03cached_items.png$append_query");
	
#	push (@cell_heading, "Cached items Ver i2C-01");
#	push (@cell_png, "controller/multisite/00.10.00.00.00.00.00.01cached_items.png$append_query");

# maybe we need to replace this	
	push (@cell_heading, "Cached items Rod i2C-03");
	push (@cell_png, "controller/multisite/00.10.00.00.00.00.00.03cached_items.png$append_query");

	push (@cell_heading, "Total number of cached items");
	push (@cell_png, "controller/cached_items.png$append_query");


	push (@cell_heading, "Interdomain IF port 08");
	push (@cell_png, "controller/multisite/01.00.00.00.00.00.00.ff/port_stat08-hour.png$append_query");

	push (@cell_heading, "Interdomain IF port 03");
	push (@cell_png, "controller/multisite/01.00.00.00.00.00.00.ff/port_stat03-hour.png$append_query");

	push (@cell_heading, "Interdomain IF port 02");
	push (@cell_png, "controller/multisite/01.00.00.00.00.00.00.ff/port_stat02-hour.png$append_query");

}


if ($global_conf eq "multi12cat") {
	my $append_query = "?&target=multi12cat";
	push (@cell_heading, "ICN client 192.168.0.2 (ETHZ)"); 
	push (@cell_png, "msi2c_et2client1_/msi2c_et2client1_eth1-hour.png$append_query");
	push (@cell_heading, "ICN server 192.168.64.2 (ETHZ)");
	push (@cell_png, "msi2c_et3server1_/msi2c_et3server1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.2 (ETHZ)");
	push (@cell_png, "msi2c_et2cache1_/msi2c_et2cache1_eth1-hour.png$append_query");

	push (@cell_heading, "ICN client 192.168.0.3 (i2cat)"); 
	push (@cell_png, "msi2c_verclient1_/msi2c_verclient1_eth1-hour.png$append_query");
	push (@cell_heading, "ICN server 192.168.64.3 (i2cat)");
	push (@cell_png, "msi2c_rodserver1_/msi2c_rodserver1_eth1-hour.png$append_query");
	push (@cell_heading, "Cache server 192.168.128.3 (i2cat)");
	push (@cell_png, "msi2c_vercache1_/msi2c_vercache1_eth1-hour.png$append_query");

	push (@cell_heading, "Cached items ETHZ2 ETH-01");
	push (@cell_png, "controller/multi12cat/02.00.00.00.00.00.00.01cached_items.png$append_query");

	push (@cell_heading, "Cached items Ver i2C-01");
	push (@cell_png, "controller/multi12cat/00.10.00.00.00.00.00.01cached_items.png$append_query");

	push (@cell_heading, "Total number of cached items");
	push (@cell_png, "controller/cached_items.png$append_query");
	
}

if ($global_conf eq "local") {
	my $file = 'host.conf';
	open my $info, $file or die "Could not open $file: $!";

#	host.conf has the following format
#	multisitel
#	3
#	cli1|192.168.0.1
#	ser1|192.168.64.1
#	cse1|192.168.128.1
#	cli2|192.168.0.2
#	ser2|192.168.64.2
#	cse2|192.168.128.2
#	cli3|192.168.0.3
#	ser3|192.168.64.3
#	cse3|192.168.128.3
#	cswiC3|0001000000000003
#	csweTHZ1|0200000000000001
	
	#my @line = <$info> # Read all lines from $file
	my $topology = "";
	
	while( my $line = <$info>)  {
		chomp($line);
	        #print "$line\n";
	        if ($. == 1) {
	        	#print "line 1 :$line\n";
       			$topology = $line;
       			next;
	        } elsif ($. == 2) {
	               	#print "line 2 :$line\n";
	               	next;
	        }
	        my @code = split(/\|/,$line); #Splits on "|"
	        my $identif = $code[0];
	        my $output_str = "";
	        my $heading = "";
	        if (substr($line,0,3) eq "cli") {
	        	$heading = "ICN client $identif";
	        	#the url is /cgi-bin/mrtg-rrd2.cgi/host_toponame/host_ipaddress-hour.png
	        	#example : /cgi-bin/mrtg-rrd2.cgi/cli1_multisitem/cli1_192.168.0.1-hour.png
	        	#the png file is /opt/lampp/htdocs/mrtg/cli1_multisitem/cli1_192.168.0.1-hour.png
	        	#the rrd file is /opt/lampp/htdocs/mrtg/cli1_multisitem/cli1_192.168.0.1.rrd
	        	
	        	$output_str = "$identif"."_"."$topology"."/$identif"."_".$code[1]."-hour.png";
	        } elsif (substr($line,0,3) eq "ser") {
	        	$heading = "ICN server $identif"; 
	        	$output_str = "$identif"."_"."$topology"."/$identif"."_".$code[1]."-hour.png";
	        } elsif (substr($line,0,3) eq "cse") {
	        	$heading = "Cache server $identif"; 
	        	$output_str = "$identif"."_"."$topology"."/$identif"."_".$code[1]."-hour.png";
	        } elsif (substr($line,0,3) eq "csw") {
	        	$heading = "Cached items ". substr($identif,3); 
	        	$output_str = add_dots_to_datapath($code[1]);
			#the url is /cgi-bin/mrtg-rrd2.cgi/controller/toponame/DATAPATHADDRESScached_items.png
			#example: /cgi-bin/mrtg-rrd2.cgi/controller/multi12cat/00.10.00.00.00.00.00.01cached_items.png
			#the png file is /opt/lampp/htdocs/mrtg/multi12cat/00.10.00.00.00.00.00.01/cacheditems-hour.png 
			#the rrd file is /opt/lampp/htdocs/mrtg/multi12cat/00.10.00.00.00.00.00.01/cacheditems.rrd 
	        	$output_str = "controller/".$topology."/$output_str"."cached_items.png";
	        } 
        	push (@cell_heading, $heading); 
        	push (@cell_png, $output_str);
	}

	close $info;

}



BEGIN {
  my $conffile = "/etc/mrtg-rrd.conf";
  $conffile = "mrtg-rrd.conf" if ( -e "mrtg-rrd.conf" );
  $conffile = $ENV{"MRTGRRDCONF"} if ( $ENV{"MRTGRRDCONF"} );
  open CONF, $conffile or die "Cannot open mrtg-rrd.conf";
  while (<CONF>) {
    chop $_;
    push @config_files, $_;
  }
close CONF;
}

$version = '0.7';

# This depends on what image format your libgd (and rrdtool) uses
$imagetype = 'png'; # or make this 'gif';

# strftime(3) compatibility test
my $percent_h = '%-H';
$percent_h = '%H' if (strftime('%-H', gmtime(0)) !~ /^\d+$/);

#this is called by the cgi handler $q contains all 
sub handler ($)
{
	my ($q) = @_;

	#print_error($q->url());
	try_read_config($q->url());

	#for example in /cgi-bin/mrtg-rrd2.cgi/multisite/01.00.00.00.00.00.00.ff/port02-hour.png
	# $path is  multisite/01.00.00.00.00.00.00.ff/port02-hour.png
	
	my $path = $q->path_info();
	$path =~ s/^\///; 
	$path =~ s/\/$//;
	#print_error($path);

	if (index($path, "controller/") == 0 || $path eq "controller") {
		#print_error("path: $path ", index($path, "controller/"));

		do_controller ($path);		
		return;
	}

	if (index($path, "controller-new/") == 0 || $path eq "controller-new") {
		#print_error("path: $path ", index($path, "controller/"));

		do_controller ($path);		
		return;
	}
	#print_error ("path: $path \n");
	#print_error ("dir(path): $directories{$path} \n");

	if (defined $directories{$path}) {

		if ($q->path_info() =~ /\/$/) {
			print_dir($path);
		} else {
			print "Location: ", $q->url(-path_info=>1), "/\n\n";
		}
		return;
	}

	# if the url is like 
	# http://10.216.12.94/cgi-bin/mrtg-rrd2.cgi/
	# http://10.216.12.94/cgi-bin/mrtg-rrd2.cgi/10.216.12.86__icn_server/
	# and the directories exists, it will not arrive here
	# if it arrives here, url is of type
	#http://10.216.12.94/cgi-bin/mrtg-rrd2.cgi/10.216.12.86__icn_server/10.216.12.86_10.216.12.86.html
	# $dir = 10.216.12.86__icn_server
	# $stat = 10.216.12.86_10.216.12.86
	
	my ($dir, $stat, $ext) = ($q->path_info() =~
		/^(.*)\/([^\/]+)(\.html|-(hour|day|week|month|year)\.$imagetype)$/);

	$dir =~ s/^\///;
	#print_error($q->url() . "\n$dir" . "\n$stat". "\n$ext");
	print_error("Undefined statistics")
		unless defined $targets{$stat};

	print_error("Incorrect directory")
		unless defined $targets{$stat}{directory} && $targets{$stat}{directory} eq $dir;

	my $tgt = $targets{$stat};

	#print_error ($targets{$stat}{directory});

	common_args($stat, $tgt, $q);

	# We may be running under mod_perl or something. Do not destroy
	# the original settings of timezone.
	my $oldtz; 
	if (defined $tgt->{timezone}) {
		$oldtz = $ENV{TZ};
		$ENV{TZ} = $tgt->{timezone};
	}

	if ($ext eq '.html') {
		do_html($tgt);
	} elsif ($ext eq '-hour.' . $imagetype) {
		do_image($tgt, 'hour');
	} elsif ($ext eq '-day.' . $imagetype) {
		do_image($tgt, 'day');
	} elsif ($ext eq '-week.' . $imagetype) {
		do_image($tgt, 'week');
	} elsif ($ext eq '-month.' . $imagetype) {
		do_image($tgt, 'month');
	} elsif ($ext eq '-year.' . $imagetype) {
		do_image($tgt, 'year');
	} else {
		print_error("Unknown extension");
	}
	$ENV{TZ} = $oldtz
		if defined $oldtz;
}

sub do_controller ($) {

	my ($path) = @_;
	
	$path = substr($path,11); 
	#print_error("path: $path ");
	
	my $index_of_cached_items = index($path, "cached_items.png"); 
	my $index_of_port_stat = index($path, "port_stat");
	my $stat_elem = "/opt/lampp/htdocs/mrtg/";

	if ( $index_of_cached_items != -1) {  
		
		#$path contains "cached_items.png"
		#either it is only cached_items.png or it is 
		#made like this multisite/00.10.00.00.00.00.00.01cached_items.png 

		if ($path eq "cached_items.png") {
			$stat_elem = $stat_elem . "controller_";
		} else {
			$stat_elem = $stat_elem . substr ($path, 0, $index_of_cached_items);
		}
		$stat_elem = $stat_elem . "/cacheditems";
		# $stat_elem = "/opt/lampp/htdocs/mrtg/controller_/cacheditems";
		# $stat_elem = "/opt/lampp/htdocs/mrtg/multisite/00.10.00.00.00.00.00.01/cacheditems";
		
		# my $stat_elem = "/opt/lampp/htdocs/mrtg/10.216.12.86__icn_server/10.216.12.86_10.216.12.86";
		# my $dataset = "ds0";
		# my $ylegend = "Bit per second"
	
		
		my $dataset = "cached_items";
		my $ylegend = "# of cached items";
	
		my $pngfile = $stat_elem . "-hour.png";
		my $rrdfile = $stat_elem . ".rrd";
		my $duration = "600"; 
		
		# my @rv = sub_graph ($pngfile, $rrdfile);
		my @rv = sub_graph_only1 ($pngfile, $rrdfile, $dataset, $ylegend, $duration);
		
		my $rrd_error = RRDs::error;
		print_error("RRDs::graph failed, $rrd_error") if defined $rrd_error;
	
		open PNG, "<$pngfile" or print_error("Can't open $pngfile: $!");
		binmode PNG;
		 
		http_headers_v2("image/png", "10", "00:30");
		 
		my $buf;
		# could be sendfile in Linux ;-)
		while(sysread PNG, $buf, 8192) {
			print $buf;
		}
		close PNG;
	} elsif ( $index_of_port_stat != -1)  { 
		# $path is made like this:
		# multisite/01.00.00.00.00.00.00.ff/port_stat02-hour.png
		$stat_elem = $stat_elem . $path ;
		
		# $stat_elem = "/opt/lampp/htdocs/mrtg/01.00.00.00.00.00.00.ff/port_stat02-hour.png";
		my $dataset = "In/out traffic";
		my $ylegend = "bit/s";
	
		my $pngfile = $stat_elem . "-hour.png";
		my $rrdfile = substr ($stat_elem , 0, -9) . ".rrd";
		#print_error ($rrdfile);
		my $duration = "600"; 
		my @rv = sub_graph_only1 ($pngfile, $rrdfile, $dataset, $ylegend, $duration);
		
		my $rrd_error = RRDs::error;
		print_error("RRDs::graph failed, $rrd_error") if defined $rrd_error;
	
		open PNG, "<$pngfile" or print_error("Can't open $pngfile: $!");
		binmode PNG;
		 
		http_headers_v2("image/png", "10", "00:30");
		 
		my $buf;
		# could be sendfile in Linux ;-)
		while(sysread PNG, $buf, 8192) {
			print $buf;
		}
		close PNG;
		
	} else {
		do_html_v2(); #prepares the traffic analysis page : /cgi-bin/mrtg-rrd2.cgi/controller/?
	}
	
}


#prepares the html page, it is called by handler if we need to prepare 
#the html page "Traffic Analysis for..."
sub do_html($)
{
	my ($tgt) = @_;

	#executes do_image in array context to load the arrays of info
	my @hour   = do_image($tgt, 'hour');
	my @day   = do_image($tgt, 'day');
	my @week  = do_image($tgt, 'week');
	my @month = do_image($tgt, 'month');
	my @year  = do_image($tgt, 'year');

	http_headers('text/html', $tgt->{config});
	print <<'EOF';
<HTML>
<HEAD>
<TITLE>
EOF
	print $tgt->{title} if defined $tgt->{title};
	print "</TITLE>\n";

	html_comments($tgt, 'h', @{$hour[0]}) if $#hour != -1;
	html_comments($tgt, 'd', @{$day[0]}) if $#day != -1;
	html_comments($tgt, 'w', @{$week[0]}) if $#week != -1;
	html_comments($tgt, 'm', @{$month[0]}) if $#month != -1;
	html_comments($tgt, 'y', @{$year[0]}) if $#year != -1;

	print $tgt->{addhead} if defined $tgt->{addhead};

	print "</HEAD>\n", $tgt->{bodytag}, "\n";
	
	print $tgt->{pagetop} if defined $tgt->{pagetop};

	unless (defined $tgt->{options}{noinfo}) {
		my @st = stat $tgt->{rrd};

		print "<HR>\nThe statistics were last updated ",
			strftime("<B>%A, %d %B, %H:%M:%S %Z</B>\n",
				localtime($st[9]));
	}

	my $dayavg = $tgt->{config}->{interval};

	html_graph($tgt, 'hour', 'Hourly', '30 Second', \@hour);
	html_graph($tgt, 'day', 'Daily', '30 Second', \@day);
	html_graph($tgt, 'week', 'Weekly', '30 Minute', \@week);
	html_graph($tgt, 'month', 'Monthly', '2 Hour', \@month);
	html_graph($tgt, 'year', 'Yearly', '1 Day', \@year);

	unless (defined $tgt->{options}{nolegend}) {
		print <<EOF;
<HR>
<table WIDTH=500 BORDER=0 CELLPADDING=4 CELLSPACING=0>
EOF
		print <<EOF unless ($tgt->{options}{noi});
    <tr>
	<td ALIGN=RIGHT><font SIZE=-1 COLOR="$tgt->{col1}">
		<b>$tgt->{colname1} ###</b></font></td>
	<td><font SIZE=-1>$tgt->{legend1}</font></td>
    </tr>
EOF
		print <<EOF unless ($tgt->{options}{noo});
    <tr>
	<td ALIGN=RIGHT><font SIZE=-1 COLOR="$tgt->{col2}">
		<b>$tgt->{colname2} ###</b></font></td>
	<td><font SIZE=-1>$tgt->{legend2}</font></td>
    </tr>
EOF
		if ($tgt->{withpeak} ne '') {
			print <<EOF unless ($tgt->{options}{noi});
    <tr>
	<td ALIGN=RIGHT><font SIZE=-1 COLOR="$tgt->{col3}">
		<b>$tgt->{colname3} ###</b></font></td>
	<td><font SIZE=-1>$tgt->{legend3}</font></td>
    </tr>
EOF
			print <<EOF unless ($tgt->{options}{noo});
    <tr>
	<td ALIGN=RIGHT><font SIZE=-1 COLOR="$tgt->{col4}">
		<b>$tgt->{colname4} ###</b></font></td>
	<td><font SIZE=-1>$tgt->{legend4}</font></td>
    </tr>
EOF
		}
		print <<EOF;
    </table>
EOF
	}

	print_banner($tgt->{config})
		unless defined $tgt->{options}{nobanner};

	print $tgt->{pagefoot} if defined $tgt->{pagefoot};
	print "\n", <<'EOF';
</body>
</html>
EOF

}

#prepares the html page, it is called by handler if we need to prepare 
#the html page "Traffic Analysis for..."
sub do_html_v2()
{
#	my ($tgt) = @_;

	# my @hour   = do_image($tgt, 'hour');
	# my @day   = do_image($tgt, 'day');
	# my @week  = do_image($tgt, 'week');
	# my @month = do_image($tgt, 'month');
	# my @year  = do_image($tgt, 'year');

	#http_headers('text/html', $tgt->{config});
	http_headers_v2("text/html", "10", "00:30");
	
	
	print <<'EOF';
<HTML>
<HEAD>
<TITLE>Evaluation of ICN Caching</TITLE> 
</HEAD>
<BODY bgcolor="#ffffff">
EOF
	
	print_header () ;
	
	# print $tgt->{pagetop} if defined $tgt->{pagetop};

	# unless (defined $tgt->{options}{noinfo}) {
	# 	my @st = stat $tgt->{rrd};

	# 	print "<HR>\nThe statistics were last updated ",
	# 		strftime("<B>%A, %d %B, %H:%M:%S %Z</B>\n",
	# 			localtime($st[9]));
	# }

	print "<H1>$web_page_header</H1>\n";
	
	
	# my $dayavg = $tgt->{config}->{interval};
	
	my $baseurl = "http://".$host_ip."/cgi-bin/mrtg-rrd2.cgi/";
	
        my $num_of_cells = scalar(@cell_heading);
	print '<table cellspacing="0" cellpadding="0">';
	print '<tbody>';
	
	for (my $count = 0; $count < $num_of_cells; $count++) {
		if (($count % $num_of_columns)== 0) {
			print '<tr>'
		}
		print '<td>'; 
		html_graph_v2($cell_heading[$count], $baseurl . $cell_png[$count]);
		print '</td>';
		if (($count % $num_of_columns)== (-1 % $num_of_columns)) {
			print '</tr>'
		}
	}
	if (($num_of_cells % $num_of_columns)!= 0) {
		print '</tr>'
	}
	

	print "\n", <<'EOF';
</tbody>
</table>


Blue line: Outgoing traffic - Green solid: Incoming traffic
</body>
</html>
EOF

}


sub html_comments($$@)
{
	my ($tgt, $letter, @val) = @_;

	return if $#val == -1;

	unless ($tgt->{options}{noi}) {
		print "<!-- maxin $letter ", $val[1], " -->\n";
		print "<!-- avin $letter ", $val[3], " -->\n";
		print "<!-- cuin $letter ", $val[5], " -->\n";
	}
	unless ($tgt->{options}{noo}) {
		print "<!-- maxout $letter ", $val[0], " -->\n";
		print "<!-- avout $letter ", $val[2], " -->\n";
		print "<!-- cuout $letter ", $val[4], " -->\n";
	}
}

# creates the html around the figures in the "Traffic analysis" page
sub html_graph($$$$$)
{
	my ($tgt, $ext, $freq, $period, $params) = @_;

	return unless defined $tgt->{$ext};

	my @values = @{$params->[0]};
	my $x = $params->[1];
	my $y = $params->[2];

	$x *= $tgt->{xzoom} if defined $tgt->{xzoom};
	$y *= $tgt->{yzoom} if defined $tgt->{yzoom};

	my $kilo = $tgt->{kilo};
	my @kmg = split(',', $tgt->{kmg});

	my $fmt;
	if (defined $tgt->{options}{integer}) {
		$fmt = '%d';
	} else {
		$fmt = '%.1f';
	}

	my @percent = do_percent($tgt, \@values);
	my @relpercent = do_relpercent($tgt, \@values);

	my @nv;
	for my $val (@values) {
		if (@kmg == 0) { # kMG[target]: <empty>
			push @nv, sprintf($fmt, $val);
			next;
		}
		for my $si (@kmg) {
			if ($val < 10000) {
				push @nv, sprintf($fmt, $val) . " $si";
				last;
			}
			$val /= $kilo;
		}
	}
	@values = @nv;

	print "<HR>\n<B>\`$freq\' Graph ($period Average)</B><BR>\n";

	print '<IMG SRC="', $tgt->{url}, '-', $ext, '.' . $imagetype .
		'" WIDTH=', $x, ' HEIGHT=', $y, ' ALT="', $freq,
		' Graph" VSPACE=10 ALIGN=TOP><BR>', "\n";
	print '<TABLE CELLPADDING=0 CELLSPACING=0>';
	print <<EOF if $tgt->{legendi} ne '' && !$tgt->{options}{noi};
    <TR>
	<TD ALIGN=RIGHT><SMALL>Max <FONT COLOR="$tgt->{col1}">$tgt->{legendi}</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[1]$tgt->{shortlegend}$percent[1]</SMALL></TD>
	<TD WIDTH=5></TD>
	<TD ALIGN=RIGHT><SMALL>Average <FONT COLOR="$tgt->{col1}">$tgt->{legendi}</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[3]$tgt->{shortlegend}$percent[3]</SMALL></TD>
	<TD WIDTH=5></TD>
	<TD ALIGN=RIGHT><SMALL>Current <FONT COLOR="$tgt->{col1}">$tgt->{legendi}</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[5]$tgt->{shortlegend}$percent[5]</SMALL></TD>
    </TR>
EOF
	print <<EOF if $tgt->{legendo} ne '' && !$tgt->{options}{noo};
    <TR>
	<TD ALIGN=RIGHT><SMALL>Max <FONT COLOR="$tgt->{col2}">$tgt->{legendo}</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[0]$tgt->{shortlegend}$percent[0]</SMALL></TD>
	<TD WIDTH=5></TD>
	<TD ALIGN=RIGHT><SMALL>Average <FONT COLOR="$tgt->{col2}">$tgt->{legendo}</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[2]$tgt->{shortlegend}$percent[2]</SMALL></TD>
	<TD WIDTH=5></TD>
	<TD ALIGN=RIGHT><SMALL>Current <FONT COLOR="$tgt->{col2}">$tgt->{legendo}</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[4]$tgt->{shortlegend}$percent[4]</SMALL></TD>
EOF
	if (defined $tgt->{options}{dorelpercent}) {
		print <<"EOF";
    </TR><TR>
	<TD ALIGN=RIGHT><SMALL>Max <FONT COLOR="$tgt->{col5}">&nbsp;Percentage:</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$relpercent[0]</SMALL></TD>
	<TD WIDTH=5></TD>
	<TD ALIGN=RIGHT><SMALL>Average <FONT COLOR="$tgt->{col5}">&nbsp;Percentage:</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$relpercent[1]</SMALL></TD>
	<TD WIDTH=5></TD>
	<TD ALIGN=RIGHT><SMALL>Current <FONT COLOR="$tgt->{col5}">&nbsp;Percentage:</FONT></SMALL></TD>
	<TD ALIGN=RIGHT><SMALL>&nbsp;$relpercent[2]</SMALL></TD>
EOF
	}
	print <<'EOF';
    </TR>
</TABLE>
EOF
}

# creates the html around the figures in the "Traffic analysis" page
sub html_graph_v2($$)
{
	# my ($tgt, $ext, $freq, $period, $params) = @_;
	my ($title, $imgurl) = @_;

	

	# my @values = @{$params->[0]};
	# my $x = $params->[1];
	# my $y = $params->[2];

	# $x *= $tgt->{xzoom} if defined $tgt->{xzoom};
	# $y *= $tgt->{yzoom} if defined $tgt->{yzoom};

	# my $kilo = $tgt->{kilo};
	# my @kmg = split(',', $tgt->{kmg});

	# my $fmt;
	# if (defined $tgt->{options}{integer}) {
	# 	$fmt = '%d';
	# } else {
	# 	$fmt = '%.1f';
	# }

	# my @percent = do_percent($tgt, \@values);
	# my @relpercent = do_relpercent($tgt, \@values);

	# my @nv;
	# for my $val (@values) {
	# 	if (@kmg == 0) { # kMG[target]: <empty>
	# 		push @nv, sprintf($fmt, $val);
	# 		next;
	# 	}
	# 	for my $si (@kmg) {
	# 		if ($val < 10000) {
	# 			push @nv, sprintf($fmt, $val) . " $si";
	# 			last;
	# 		}
	# 		$val /= $kilo;
	# 	}
	# }
	# @values = @nv;

	print "<HR>\n<B>$title</B><BR>\n";

# smaller image for 1024x768 (projector) screens
	print '<IMG SRC="', $imgurl ,
		'" WIDTH=397 HEIGHT=123 ALT="', $title,
		'" VSPACE=10 ALIGN=TOP><BR>', "\n";
#larger image	
#	print '<IMG SRC="', $imgurl ,
#		'" WIDTH=497 HEIGHT=154 ALT="', $title,
#		'" VSPACE=10 ALIGN=TOP><BR>', "\n";
	print '<TABLE CELLPADDING=0 CELLSPACING=0>';
# 	print <<EOF if $tgt->{legendi} ne '' && !$tgt->{options}{noi};
#     <TR>
# 	<TD ALIGN=RIGHT><SMALL>Max <FONT COLOR="$tgt->{col1}">$tgt->{legendi}</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[1]$tgt->{shortlegend}$percent[1]</SMALL></TD>
# 	<TD WIDTH=5></TD>
# 	<TD ALIGN=RIGHT><SMALL>Average <FONT COLOR="$tgt->{col1}">$tgt->{legendi}</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[3]$tgt->{shortlegend}$percent[3]</SMALL></TD>
# 	<TD WIDTH=5></TD>
# 	<TD ALIGN=RIGHT><SMALL>Current <FONT COLOR="$tgt->{col1}">$tgt->{legendi}</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[5]$tgt->{shortlegend}$percent[5]</SMALL></TD>
#     </TR>
# EOF
# 	print <<EOF if $tgt->{legendo} ne '' && !$tgt->{options}{noo};
#     <TR>
# 	<TD ALIGN=RIGHT><SMALL>Max <FONT COLOR="$tgt->{col2}">$tgt->{legendo}</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[0]$tgt->{shortlegend}$percent[0]</SMALL></TD>
# 	<TD WIDTH=5></TD>
# 	<TD ALIGN=RIGHT><SMALL>Average <FONT COLOR="$tgt->{col2}">$tgt->{legendo}</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[2]$tgt->{shortlegend}$percent[2]</SMALL></TD>
# 	<TD WIDTH=5></TD>
# 	<TD ALIGN=RIGHT><SMALL>Current <FONT COLOR="$tgt->{col2}">$tgt->{legendo}</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$values[4]$tgt->{shortlegend}$percent[4]</SMALL></TD>
# EOF
# 	if (defined $tgt->{options}{dorelpercent}) {
# 		print <<"EOF";
#     </TR><TR>
# 	<TD ALIGN=RIGHT><SMALL>Max <FONT COLOR="$tgt->{col5}">&nbsp;Percentage:</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$relpercent[0]</SMALL></TD>
# 	<TD WIDTH=5></TD>
# 	<TD ALIGN=RIGHT><SMALL>Average <FONT COLOR="$tgt->{col5}">&nbsp;Percentage:</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$relpercent[1]</SMALL></TD>
# 	<TD WIDTH=5></TD>
# 	<TD ALIGN=RIGHT><SMALL>Current <FONT COLOR="$tgt->{col5}">&nbsp;Percentage:</FONT></SMALL></TD>
# 	<TD ALIGN=RIGHT><SMALL>&nbsp;$relpercent[2]</SMALL></TD>
# EOF
# 	}
	print <<'EOF';
    </TR>
</TABLE>
EOF
}


sub do_percent($$)
{
	my ($tgt, $values) = @_;

	my @percent = ('', '', '', '', '', '');

	return @percent if defined $tgt->{options}{nopercent};

	for my $val (0..$#$values) {
		my $mx = ($val % 2 == 1) ? $tgt->{maxbytes1} : $tgt->{maxbytes2};
		next unless defined $mx;
		my $p = sprintf("%.1f", $values->[$val]*100/$mx);
		$percent[$val] = ' (' . $p . '%)';
	}

	@percent;
}

sub do_relpercent($$)
{
	my ($tgt, $values) = @_;

	my @percent = ('', '', '');

	return @percent unless defined $tgt->{options}{dorelpercent};

	for my $val (0..2) {
		$percent[$val] = sprintf("%.1f", 
			$values->[2*$val+1] * 100 / $values->[2*$val])
			if $values->[2*$val] > 0;
		$percent[$val] ||= 0;
		$percent[$val] .= ' %';
	}

	@percent;
}

sub http_headers($$)
{
	my ($content_type, $cfg) = @_;

	http_headers_v2($content_type, $cfg->{refresh}, $cfg->{interval});
}

# this sub takes in input refresh and interval instead of $cfg
sub http_headers_v2($$$)
{
	my ($content_type, $refresh, $interval) = @_;

	print <<"EOF";
Content-Type: $content_type
Refresh: $refresh
Pragma: no-cache
EOF
	# Expires header calculation stolen from CGI.pm
	print strftime("Expires: %a, %d %b %Y %H:%M:%S GMT\n",
		gmtime(time+60*$interval));

	print "\n";
}

#prepares the images
sub do_image($$)
{
	my ($target, $ext) = @_;

	my $file = $target->{$ext};

	return unless defined $file;

	# Now the vertical rule at the end of the day
	my @t = localtime(time);
	$t[0] = $t[1] = $t[2] = 0;

	my $seconds;
	my $oldsec;
	my $back;
	my $xgrid;

	my $unscaled;
	my $withpeak;

	my $noi = 1 if $target->{options}{noi};
	my $noo = 1 if $target->{options}{noo};

	if ($ext eq 'hour') {
		$seconds = timelocal(@t);
		#$back = 2*3600;	# 2 hours
		$back = 600;	# 10 min
		$oldsec = $seconds - 7000;
		$unscaled = 1 if $target->{unscaled} =~ /d/;
		$withpeak = 1 if $target->{withpeak} =~ /d/;
	} elsif ($ext eq 'day') {
		$seconds = timelocal(@t);
		$back = 30*3600;	# 30 hours
		$oldsec = $seconds - 86400;
		$unscaled = 1 if $target->{unscaled} =~ /d/;
		$withpeak = 1 if $target->{withpeak} =~ /d/;
		# We need this only for day graph. The other ones
		# are magically correct.
		$xgrid = 'HOUR:1:HOUR:6:HOUR:2:0:' . $percent_h;
	} elsif ($ext eq 'week') {
		$seconds = timelocal(@t);
		$t[6] = ($t[6]+6) % 7;
		$seconds -= $t[6]*86400;
		$back = 8*86400;	# 8 days
		$oldsec = $seconds - 7*86400;
		$unscaled = 1 if $target->{unscaled} =~ /w/;
		$withpeak = 1 if $target->{withpeak} =~ /w/;
	} elsif ($ext eq 'month') {
		$t[3] = 1;
		$seconds = timelocal(@t);
		$back = 36*86400;	# 36 days
		$oldsec = $seconds - 30*86400; # FIXME (the right # of days!!)
		$unscaled = 1 if $target->{unscaled} =~ /m/;
		$withpeak = 1 if $target->{withpeak} =~ /m/;
	} elsif ($ext eq 'year') {
		$t[3] = 1;
		$t[4] = 0;
		$seconds = timelocal(@t);
		$back = 396*86400;	# 365 + 31 days
		$oldsec = $seconds - 365*86400; # FIXME (the right # of days!!)
		$unscaled = 1 if $target->{unscaled} =~ /y/;
		$withpeak = 1 if $target->{withpeak} =~ /y/;
	} else {
		print_error("Unknown file extension: $ext");
	}

	my @local_args;

	if ($unscaled) {
		@local_args = ('-u', $target->{maxbytes1});
		push @local_args, '--rigid' unless defined $target->{absmax};
	}

	if ($xgrid) {
		push @local_args, '-x', $xgrid;
	}

	my @local_args_end;

	if ($withpeak) {
		push @local_args_end, 'LINE1:maxin'.$target->{col3}.':MaxIn'
			unless $noi;
		push @local_args_end, 'LINE1:maxout'.$target->{col4}.':MaxOut'
			unless $noo;
	}

	my @rv = RRDs::graph($file, '-s', "-$back", @local_args,
		@{$target->{args}}, "VRULE:$oldsec#ff0000",
		"VRULE:$seconds#ff0000", @local_args_end);

	my $rrd_error = RRDs::error;
	print_error("RRDs::graph failed, $rrd_error") if defined $rrd_error;

	#the following line will print all the graph parameters
	#	print_error("\nRRD::graph parameters:"
	#	    . "\n$file" 
	#	    . "\n-s -$back"
	#	    . "\n@local_args"
	#	    . "\n@{$target->{args}}"
	#	    . "\nVRULE:$oldsec#ff0000"
	#	    . "\nVRULE:$seconds#ff0000"
	#	    . "\n@local_args_end"
	#	    );

	# In array context just return the values
	if (wantarray) {
		if (defined $target->{factor}) {
			@{$rv[0]} = map { $_ * $target->{factor} } @{$rv[0]};
		}
		if ($noi) {
			return ([$rv[0][0], 0, $rv[0][1], 0, $rv[0][2], 0],
				$rv[1], $rv[2]);
		} elsif ($noo) {
			return ([0, $rv[0][0], 0, $rv[0][1], 0, $rv[0][2]],
				$rv[1], $rv[2]);
		} else {
			return @rv;
		}
	}

	# Not in array context ==> print out the PNG file.
	open PNG, "<$file" or print_error("Can't open $file: $!");

	binmode PNG;

#	my $hash = $target->{config};
#	my $outs = "";
#	foreach my $key (keys %$hash){
#		$outs = $outs . "$key = $$hash{$key}\n";
#	}
#	print_error ("ciao $outs");
	
	http_headers("image/$imagetype", $target->{config});
		
	my $buf;
        # could be sendfile in Linux ;-)
        while(sysread PNG, $buf, 8192) {
                print $buf;
        }
	close PNG;
}

# draw the png of a single line (e.g. used for cached items)
sub sub_graph_only1 ($$$$$) {
	# 
        my ($pngfile, $rrdfile, $dataset, $ylegend, $duration) = @_;
 
	my @rv = RRDs::graph(
$pngfile,
'-s',
"-$duration",
# "--step",
# "15",
"--lazy",
"-c",
"FONT#000000",
"-c",
"MGRID#000000",
"-c",
"FRAME#000000",
"-g",
"-l",
"0",
"-c",
"BACK#f5f5f5",
"-c",
"ARROW#000000",
"-b",
"1000",
"-w",
"400",
"-h",
"100",
"DEF:in=$rrdfile:$dataset:AVERAGE",
# "CDEF:in=in0,8,*",
# "DEF:maxin0=$rrdfile:$dataset:MAX",
# "CDEF:maxin=maxin0,8,*",
# "DEF:out0=$rrdfile:ds1:AVERAGE",
# "CDEF:out=out0,8,*",
# "DEF:maxout0=$rrdfile:ds1:MAX",
# "CDEF:maxout=maxout0,8,*",
"-v",
"$ylegend",
#"AREA:in#00cc00:In",
"LINE3:in#0000ff:In",
#"LINE3:out#0000ff:Out",
# "PRINT:out:MAX:%.1lf",
# "PRINT:in:MAX:%.1lf",
# "PRINT:out:AVERAGE:%.1lf",
"PRINT:in:AVERAGE:%.1lf",
# "PRINT:out:LAST:%.1lf",
"PRINT:in:LAST:%.1lf",
# "HRULE:1000000000#cc0000",
# "HRULE:100#cc0000",
# "VRULE:1336075400#ff0000",
# "VRULE:1336082400#ff0000",
);
		return @rv; 
}
 
# draw the png of a mrtg RDD, with 7200 second duration
# it works for /opt/lampp/htdocs/mrtg/10.216.12.86__icn_server/10.216.12.86_10.216.12.86
sub sub_graph ($$) {
	# 
        my ($pngfile, $rrdfile) = @_;
 
	my @rv = RRDs::graph(
$pngfile,
'-s',
"-7200",
"--lazy",
"-c",
"FONT#000000",
"-c",
"MGRID#000000",
"-c",
"FRAME#000000",
"-g",
"-l",
"0",
"-c",
"BACK#f5f5f5",
"-c",
"ARROW#000000",
"-b",
"1000",
"-w",
"400",
"-h",
"100",
"DEF:in0=$rrdfile:ds0:AVERAGE",
"CDEF:in=in0,8,*",
"DEF:maxin0=$rrdfile:ds0:MAX",
"CDEF:maxin=maxin0,8,*",
"DEF:out0=$rrdfile:ds1:AVERAGE",
"CDEF:out=out0,8,*",
"DEF:maxout0=$rrdfile:ds1:MAX",
"CDEF:maxout=maxout0,8,*",
"-v",
"Bit per second",
"AREA:in#00cc00:In",
"LINE3:out#0000ff:Out",
"PRINT:out:MAX:%.1lf",
"PRINT:in:MAX:%.1lf",
"PRINT:out:AVERAGE:%.1lf",
"PRINT:in:AVERAGE:%.1lf",
"PRINT:out:LAST:%.1lf",
"PRINT:in:LAST:%.1lf",
"HRULE:1000000000#cc0000",
"VRULE:1336075400#ff0000",
"VRULE:1336082400#ff0000");

		return @rv; 
}


#prepares the arguments 
sub common_args($$$)
{
	my ($name, $target, $q) = @_;

	return @{$target->{args}} if defined @{$target->{args}};

	my $noi = 1 if $target->{options}{noi};
	my $noo = 1 if $target->{options}{noo};

	$target->{name} = $name;

	$target->{directory} = ''
		unless defined $target->{directory};

	my $tdir = $target->{directory};
	$tdir .= '/'
		unless $tdir eq '' || $tdir =~ /\/$/;

	$target->{url} = $q->url . '/' . $tdir . $name;

	my $cfg = $target->{config};

	my $dir = $cfg->{workdir};
	$dir = $cfg->{logdir}
		if defined $cfg->{logdir};

	$target->{rrd} = $dir . '/' . $tdir . $name . '.rrd';

	%{$target->{options}} = ()
		unless defined %{$target->{options}};

	$dir = $cfg->{workdir};
	$dir = $cfg->{imagedir}
		if defined $cfg->{imagedir};

	$target->{suppress} ||= '';

	$target->{hour}   = $dir . '/' . $tdir . $name
		. '-hour.' . $imagetype unless $target->{suppress} =~ /h/;
	$target->{day}   = $dir . '/' . $tdir . $name
		. '-day.' . $imagetype unless $target->{suppress} =~ /d/;
	$target->{week}  = $dir . '/' . $tdir . $name
		. '-week.' . $imagetype unless $target->{suppress} =~ /w/;
	$target->{month} = $dir . '/' . $tdir . $name
		. '-month.' . $imagetype unless $target->{suppress} =~ /m/;
	$target->{year}  = $dir . '/' . $tdir . $name
		. '-year.' . $imagetype unless $target->{suppress} =~ /y/;

	$target->{maxbytes1} = $target->{maxbytes}
		if defined $target->{maxbytes} && !defined $target->{maxbytes1};

	$target->{maxbytes2} = $target->{maxbytes1}
		if defined $target->{maxbytes1} && !defined $target->{maxbytes2};

	my @args = ();

	push @args, '--lazy', '-c', 'FONT#000000', '-c',
		'MGRID#000000', '-c', 'FRAME#000000',
		'-g', '-l', '0';

	$target->{background} = '#f5f5f5'
		unless defined $target->{background};

	push @args, '-c', 'BACK'. $target->{background};

	push @args, '-c', 'SHADEA' . $target->{background},
		'-c', 'SHADEB' . $target->{background}
		if defined $target->{options}{noborder};

	if (defined $target->{options}{noarrow}) {
		push @args, '-c', 'ARROW' . $target->{background};
	} else {
		push @args, '-c', 'ARROW#000000';
	}

	push @args, '-b', $target->{kilo}
		if defined $target->{kilo};

	if (defined $target->{xsize}) {
		if (defined $target->{xscale}) {
			push @args, '-w', $target->{xsize}*$target->{xscale};
		} else {
			push @args, '-w', $target->{xsize};
		}
	}

	if (defined $target->{ysize}) {
		if (defined $target->{yscale}) {
			push @args, '-h', $target->{ysize}*$target->{yscale};
		} else {
			push @args, '-h', $target->{ysize};
		}
	}

	my $scale = 1;
	
	if (defined $target->{options}->{perminute}) {
		$scale *= 60;
	} elsif (defined $target->{options}->{perhour}) {
		$scale *= 3600;
	}

	if (defined $target->{options}->{bits}) {
		$scale *= 8;
		$target->{ylegend} = 'Bits per second'
			unless defined $target->{ylegend};
		$target->{legend1} = 'Incoming Traffic in Bits per Second'
			unless defined $target->{legend1};
		$target->{legend2} = 'Outgoing Traffic in Bits per Second'
			unless defined $target->{legend2};
		$target->{legend3} = 'Peak Incoming Traffic in Bits per Second'
			unless defined $target->{legend3};
		$target->{legend4} = 'Peak Outgoing Traffic in Bits per Second'
			unless defined $target->{legend4};
		$target->{shortlegend} = 'b/s'
			unless defined $target->{shortlegend};
	} else {
		$target->{ylegend} = 'Bytes per second'
			unless defined $target->{ylegend};
		$target->{legend1} = 'Incoming Traffic in Bytes per Second'
			unless defined $target->{legend1};
		$target->{legend2} = 'Outgoing Traffic in Bytes per Second'
			unless defined $target->{legend2};
		$target->{legend3} = 'Peak Incoming Traffic in Bytes per Second'
			unless defined $target->{legend3};
		$target->{legend4} = 'Peak Outgoing Traffic in Bytes per Second'
			unless defined $target->{legend4};
		$target->{shortlegend} = 'B/s'
			unless defined $target->{shortlegend};
	}

	if ($scale > 1) {
		push @args, "DEF:in0=$target->{rrd}:ds0:AVERAGE",
			"CDEF:in=in0,$scale,*",
			"DEF:maxin0=$target->{rrd}:ds0:MAX",
			"CDEF:maxin=maxin0,$scale,*"
			unless $noi;
		push @args, "DEF:out0=$target->{rrd}:ds1:AVERAGE",
			"CDEF:out=out0,$scale,*",
			"DEF:maxout0=$target->{rrd}:ds1:MAX",
			"CDEF:maxout=maxout0,$scale,*"
			unless $noo;
	} else {
		push @args, "DEF:in=$target->{rrd}:ds0:AVERAGE",
			"DEF:maxin=$target->{rrd}:ds0:MAX"
			unless $noi;
		push @args, "DEF:out=$target->{rrd}:ds1:AVERAGE",
			"DEF:maxout=$target->{rrd}:ds1:MAX"
			unless $noo;
	}

	my $i=1;
	for my $coltext (split(/,/, $target->{colours})) {
		my ($text, $rgb) = ($coltext =~ /^([^#]+)(#[0-9a-fA-F]{6})$/);
		$target->{'col'.$i} = $rgb;
		$target->{'colname'.$i} = $text;
		$i++;
	}

	push @args, '-v', $target->{ylegend};

	push @args, 'AREA:in' . $target->{col1} . ':In',
		unless $noi;
	push @args, 'LINE3:out' . $target->{col2} . ':Out'
		unless $noo;

	push @args, 'PRINT:out:MAX:%.1lf' unless $noo;
	push @args, 'PRINT:in:MAX:%.1lf'  unless $noi;
	push @args, 'PRINT:out:AVERAGE:%.1lf' unless $noo;
	push @args, 'PRINT:in:AVERAGE:%.1lf'  unless $noi;
	push @args, 'PRINT:out:LAST:%.1lf' unless $noo;
	push @args, 'PRINT:in:LAST:%.1lf'  unless $noi;

	if (defined $target->{maxbytes1}) {
		$target->{maxbytes1} *= $scale;
		push @args, 'HRULE:' . $target->{maxbytes1} . '#cc0000';
	}

	if (defined $target->{maxbytes2}) {
		$target->{maxbytes2} *= $scale;
		push @args, 'HRULE:' . $target->{maxbytes2} . '#cccc00'
			if $target->{maxbytes2} != $target->{maxbytes1};
	}

	@{$target->{args}} = @args;

	@args;
}

# reads the configuration files
sub try_read_config($)
{
	my ($prefix) = (@_);
	$prefix =~ s/\/[^\/]*$//;

	# Verify the version of RRDtool:
	if (!defined $RRDs::VERSION || $RRDs::VERSION < 1.000331) {
		print_error("Please install more up-to date RRDtool - need at least 1.000331");
	}
	
	my $read_cfg;
	if (!defined $config_time) {
		$read_cfg = 1;
	} else {
		for my $file (@all_config_files) {
			my @stat = stat $file;
			if ($config_time < $stat[9]) {
				$read_cfg = 1;
				last;
			}
		}
	}

	return unless $read_cfg;

	my %defaults = (
		xsize => 400,
		ysize => 100,
		kmg => ',k,M,G,T,P',
		kilo => 1000,
		bodytag => "<BODY BGCOLOR=#ffffff>\n",
		colours => 'GREEN#00cc00,BLUE#0000ff,DARK GREEN#006600,MAGENTA#ff00ff,AMBER#ef9f4f',
		legendi => '&nbsp;In:',
		legendo => '&nbsp;Out:',
		unscaled => '',
		withpeak => '',
		directory => '',
	);

	%targets = ();

	@all_config_files = @config_files;

	my $order = 0;
	for my $cfgfile (@config_files) {
		%{$targets{_}} = %defaults;
		%{$targets{'^'}} = ();
		%{$targets{'$'}} = ();

#			refresh => 300,
		my $cfgref = {
			refresh => 10,
			interval => 5,
			icondir => $prefix
		};

		read_mrtg_config($cfgfile, \%defaults, $cfgref, \$order);
	}

	delete $targets{'^'};
	delete $targets{_};
	delete $targets{'$'};

	parse_directories();

	$config_time = time;
}

sub read_mrtg_config($$$$);

sub read_mrtg_config($$$$)
{
	my ($file, $def, $cfgref, $order) = @_;

	my %defaults = %$def;

	my @lines;

	open(CFG, "<$file") || print_error("Cannot open config file: $!");
	while (<CFG>) {
		chomp;                    # remove newline
		s/\s+$//;                 # remove trailing space
		s/\s+/ /g;                # collapse white spaces to ' '
		next if /^ *\#/;           # skip comment lines
		next if /^\s*$/;          # skip empty lines
		if (/^ \S/) {             # multiline options
			$lines[$#lines] .= $_;
		} else {
			push @lines, $_;
		}
	}
	close CFG;

	foreach (@lines) {
		if (/^\s*([\w\d]+)\[(\S+)\]\s*:\s*(.*)$/) {
			my ($tgt, $opt, $val) = (lc($2), lc($1), $3);
			unless (exists $targets{$tgt}) {
				# pre-set defaults constructed on all of ^, _, and $
				for my $key (%{$targets{'^'}}) {
					$targets{$tgt}{$key} = $targets{'^'}{$key};
				}
				for my $key (%{$targets{'$'}}) {
					$targets{$tgt}{$key} .= $targets{'$'}{$key};
				}
				# WARNING: Tobi explicitly said that when all ^, _, and $
				# options are set, the result should be just the value
				# of the _ option (when the option itself is not explicitly
				# defined. I do not agree with him here but I respect this
				# and will try to be compatible with MRTG.
				for my $key (%{$targets{'_'}}) {
					$targets{$tgt}{$key} = $targets{'_'}{$key};
				}

				# anonymous hash ref - need copy, not ref
				delete $targets{$tgt}{options};
				# The same as above - we need to create this
				# based on [^], [_], and [$] values
				%{$targets{$tgt}{options}} = ();
				%{$targets{$tgt}{options}} = %{$targets{'^'}{options}}
					if defined $targets{'^'}{options};
				%{$targets{$tgt}{options}} = (%{$targets{$tgt}{options}},
					%{$targets{'_'}{options}})
					if defined $targets{'_'}{options};
				%{$targets{$tgt}{options}} = (%{$targets{$tgt}{options}},
					%{$targets{'$'}{options}})
					if defined $targets{'$'}{options};

				$targets{$tgt}{order} = ++$$order;
				$targets{$tgt}{config} = $cfgref;
			}
			if ($tgt eq '_' && $val eq '') {
				if (defined $defaults{$opt}) {
					$targets{_}{$opt} = $defaults{$opt};
				} else {
					delete $targets{_}{$opt};
				}
			} elsif (($tgt eq '^' || $tgt eq '$') && $val eq '') {
				delete $targets{$tgt}{$opt};
			} elsif ($opt eq 'options') {
				# Do not forget defaults [^] and [$]
				delete $targets{$tgt}{options};
				%{$targets{$tgt}{options}} = %{$targets{'^'}{options}}
					if defined $targets{'^'}{options};
				$val = lc($val);
				map { $targets{$tgt}{options}{$_} = 1 } ($val =~ m/([a-z]+)/g);
				%{$targets{$tgt}{options}} = (%{$targets{$tgt}{options}},
					%{$targets{'$'}{options}})
					if defined $targets{'$'}{options};
			} else {
				my $pre = $targets{'^'}{$opt}
					if defined $targets{'^'}{$opt};
				$pre ||= '';
				$targets{$tgt}{$opt} = $pre.$val;
				$targets{$tgt}{$opt} .= $targets{'$'}{$opt}
					if defined $targets{'$'}{$opt};
			}
			next;
		} elsif (/^Include\s*:\s*(\S*)$/) {
			push @all_config_files, $1;
			read_mrtg_config($1, $def, $cfgref, $order);
			next;
		} elsif (/^([\w\d]+)\s*:\s*(\S.*)$/) {
			my ($opt, $val) = (lc($1), $2);
			$cfgref->{$opt} = $val;
			next;
		}
		print_error("Parse error in $file near $_");
	}

	if (defined $cfgref->{pathadd}) {
		$ENV{PATH} .= ':'.$cfgref->{pathadd};
	}

#	if (defined $cfgref->{libadd}) {
#		use lib $cfgref->{libadd}
#	}
}

#creates the %directories hash ; it is called by try_read_config
sub parse_directories {
	%directories = ();

	# FIXME: the sort is expensive
	for my $name (sort { $targets{$a}{order} <=> $targets{$b}{order} } keys %targets) {
		my $dir = $targets{$name}{directory}
			if defined $targets{$name}{directory};
		$dir = '' unless defined $dir;

		my $prefix = '';
		for my $component (split /\/+/, $dir) {
			unless (defined $directories{$prefix.$component}) {
				push (@{$directories{$prefix}{subdir}},
					$component);

				# For the directory, get the global parameters
				# from the # config of the first item of the
				# directory:
				$directories{$prefix}{config} =
					$targets{$name}{config};
				$directories{$prefix}{bodytag} =
					$targets{$name}{bodytag};
			}
			$prefix .= $component . '/';
		}
		unless (defined $directories{$dir}) {
			$directories{$dir}{config} =
				$targets{$name}{config};
			$directories{$dir}{bodytag} =
				$targets{$name}{bodytag};
		}

		push (@{$directories{$dir}{target}}, $name);
	}
}

# print_dir is used to create the html pages "MRTG subdirectories in the directory /
# and MRTG graphs in the directory
# it is called by sub handler

sub print_dir($) {
	my ($dir) = @_;

	my $dir1 = $dir . '/';

	http_headers('text/html', $directories{$dir}{config});

	print <<EOF;
<HTML>
<HEAD>
<TITLE>MRTG: Directory $dir1</TITLE>
</HEAD>
EOF
	print $directories{$dir}{bodytag};

	my $subdirs_printed;
	if (defined @{$directories{$dir}{subdir}}) {
		$subdirs_printed = 1;
		print <<EOF;
<H1>MRTG subdirectories in the directory $dir1</H1>

<UL>
EOF
		for my $item (@{$directories{$dir}{subdir}}) {
			print "<LI><A HREF=\"$item/\">$item/</A>\n";
		}

		print "</UL>\n";
	}
	if (defined @{$directories{$dir}{target}}) {
		print "<HR>\n" if defined $subdirs_printed;
		print <<EOF;
<H1>MRTG graphs in the directory $dir1</H1>

<TABLE BORDER=0 WIDTH=100%>
EOF
		my $odd;
		for my $item (@{$directories{$dir}{target}}) {
			my $itemname = $item;
			$itemname = $targets{$item}{title}
				if defined $targets{$item}{title};
			print "    <TR>\n" unless $odd;
			print <<EOF;
   <TD><A HREF="$item.html">$itemname<BR>
	<IMG SRC="$item-day.$imagetype" BORDER=0 ALIGN=TOP VSPACE=10 ALT="$item">
	</A><BR CLEAR=ALL>
   </TD>
EOF
			print "    </TR>\n" if $odd;
			$odd = !$odd;
		} 
		print "    </TR>\n</TABLE>\n";
	}

	print_banner($directories{$dir}{config});
	print "</BODY>\n</HTML>\n";
}

sub print_banner($) {
	my $cfg = shift;

	print <<EOF;

<HR>
<table BORDER=0 CELLSPACING=0 CELLPADDING=0>
<tr>
<td WIDTH=63><a ALT="MRTG"
    HREF="http://ee-staff.ethz.ch/~oetiker/webtools/mrtg/mrtg.html"><img
BORDER=0 SRC="$cfg->{icondir}/mrtg-l.$imagetype"></a></td>
<td WIDTH=25><a ALT=""
    HREF="http://ee-staff.ethz.ch/~oetiker/webtools/mrtg/mrtg.html"><img
BORDER=0 SRC="$cfg->{icondir}/mrtg-m.$imagetype"></a></td>
<td WIDTH=388><a ALT=""
    HREF="http://ee-staff.ethz.ch/~oetiker/webtools/mrtg/mrtg.html"><img
BORDER=0 SRC="$cfg->{icondir}/mrtg-r.$imagetype"></a></td>
</tr>
</table>
<spacer TYPE=VERTICAL SIZE=4>
<table BORDER=0 CELLSPACING=0 CELLPADDING=0 WIDTH=476>
<tr VALIGN=top>
<td ALIGN=LEFT><font FACE="Arial,Helvetica" SIZE=2>
version 2.9.17</font></td>
<td ALIGN=RIGHT><font FACE="Arial,Helvetica" SIZE=2>
<a HREF="http://ee-staff.ethz.ch/~oetiker/">Tobias Oetiker</a>
<a HREF="mailto:oetiker\@ee.ethz.ch">&lt;oetiker\@ee.ethz.ch&gt;</a>
</font></td>
</tr><tr>
<td></td>
<td ALIGN=RIGHT><font FACE="Arial,Helvetica" SIZE=2>
and&nbsp;<a HREF="http://www.bungi.com">Dave&nbsp;Rand</a>&nbsp;<a HREF="mailto:dlr\@bungi.com">&lt;dlr\@bungi.com&gt;</a></font>
</td>
<tr VALIGN=top>
<td ALIGN=LEFT><font FACE="Arial,Helvetica" SIZE=2>
<A HREF=http://www.fi.muni.cz/~kas/mrtg-rrd/>mrtg-rrd.cgi version $version</A>
</font></td>
<td ALIGN=RIGHT><font FACE="Arial,Helvetica" SIZE=2>
<A HREF="http://www.fi.muni.cz/~kas/">Jan "Yenya" Kasprzak</A>
<A HREF="mailto:kas\@fi.muni.cz">&lt;kas\@fi.muni.cz&gt;</A>
</font></td>
</tr>
</table>
EOF
	print '<!--$Id: mrtg-rrd.cgi,v 1.35 2003/08/18 15:58:57 kas Exp $-->', "\n";
}

sub dump_targets() {
	for my $tgt (keys %targets) {
		print "Target $tgt:\n";
		for my $opt (keys %{$targets{$tgt}}) {
			if ($opt eq 'options') {
				print "\toptions: ";
				for my $o1 (keys %{$targets{$tgt}{options}}) {
					print $o1, ",";
				}
				print "\n";
				next;
			}
			print "\t$opt: ", $targets{$tgt}{$opt}, "\n";
		}
	}
}

sub dump_directories {
	print "Directories:\n";

	for my $dir (keys %directories) {
		print "Directory $dir:\n";
		for my $item (@{$directories{$dir}}) {
			print "\t$item\n";
		}
	}
}

sub print_error(@)
{
	print "Content-Type: text/plain\n\nError: ", join(' ', @_), "\n";
	exit 0;
}

#--BEGIN CGI--
#For CGI, use this:

use CGI;
my $q = new CGI;

# thttpd fix up by Akihiro Sagawa
if ($q->server_software() =~ m|^thttpd/|) {
	my $path = $q->path_info();
	$path .= '/' if ($q->script_name=~ m|/$|);
	$q->path_info($path);
}

handler($q);

#--END CGI--
#--BEGIN FCGI--
# For FastCGI, uncomment this and comment out the above:
#-# use FCGI;
#-# use CGI;
#-# 
#-# my $req = FCGI::Request();
#-# 
#-# while ($req->Accept >= 0) {
#-# 	my $q = new CGI;
#-# 
#-# 	# thttpd fix up by Akihiro Sagawa
#-# 	if ($q->server_software() =~ m|^thttpd/|) {
#-# 		my $path = $q->path_info();
#-# 		$path .= '/' if ($q->script_name=~ m|/$|);
#-# 		$q->path_info($path);
#-# 	}
#-# 
#-# 	handler($q);
#-# }
#--END FCGI--

1;

#only works when datapath is expressed as a string of 16 chars
sub add_dots_to_datapath {
	my $input_str = $_[0];
	#print "input string: $input_str\n";
	my $output_str = "";
	my $i=0;
	for (; $i < 7; $i++) {
		$output_str .=  substr ($input_str, 2*$i, 2).".";
	}
	$output_str .=  substr ($input_str, 2*$i, 2);
	return $output_str;
}


sub print_header () {
print '<table style="text-align: left; width: 100%; background-color: rgb(215, 255, 255);" border="0" cellpadding="0" cellspacing="0">';
print '  <tbody><tr>';
print '      <td align="undefined" valign="undefined"><a href="http://www.fp7-ofelia.eu/">';
print '      <img style="border: 0px solid ;" alt="" src="/ofelia-logo.png"></a></td>';
print '      <td style="text-align: right;" valign="undefined"><a href="http://www.cnit.it/">';
print '      <img style="border: 0px solid ;" alt="" src="/cnit-logo.png"></a></td>';
print '   </tr></tbody>';
print '</table>';
}

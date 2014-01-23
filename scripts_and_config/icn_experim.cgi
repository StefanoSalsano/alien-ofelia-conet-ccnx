#!/usr/bin/perl -w

# in our configuration, it is stored in /opt/lampp/cgi-bin/
# wget https://www.dropbox.com/s/01okpfcqlmz1zty/icn_experim.cgi

# simple local test: /usr/bin/perl -w mrtg-rrd2.cgi
# local test with parameters using the shell script
# /opt/lampp/cgi-bin/test-icn_experim.sh
# wget https://www.dropbox.com/s/a0sxesfxax0so7a/test-icn_experim.sh

use strict;
use WWW::Curl::Easy;
use JSON::XS;
use HTML::QuickTable;
use MIME::Base64;

#my $global_conf = "i2cat";
#my $global_conf = "multi12cat";
#my $global_conf = "trento";
my $global_conf = "local";

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
# multi12cat (barcellona multi site) or ""
my $target = $FORM{target};

#$tagbasedfw can be activate / deactivate / or "" 
my $tagbasedfw = $FORM{tagbasedfw};

#$queryfor can be "" / "flowtable" / "cachedcontents"
my $queryfor = $FORM{queryfor};

#when used with flowtable $querysw can be "" /  "all" / 00:10:00:00:00:00:00:05 / any other sw address
#when used with cachedcontents $querysw can be "" /  "all" / MAC address of the cache server : THIS IS NOT YET IMPLEMENTED
my $querysw = $FORM{querysw};

# end reading URL parameters
###########################################################

# redefines $global_conf from target URL parameter if present
if (! ($target eq "") ) {
	$global_conf = $target;
}

my $web_page_header = "ICN experiment in testbed : ";
$web_page_header = $web_page_header. " OFELIA (slice i2cat)" if ($global_conf eq "i2cat");
$web_page_header = $web_page_header. " OFELIA (slice multisite)" if ($global_conf eq "trento");
$web_page_header = $web_page_header. " OFELIA (slice multisite small)" if ($global_conf eq "multi12cat");
$web_page_header = $web_page_header. " local " if ($global_conf eq "local");



my $VLAN_ID = 0;

$VLAN_ID = 16 if ($global_conf eq "i2cat");
$VLAN_ID = 3001 if ($global_conf eq "trento");
$VLAN_ID = 801 if ($global_conf eq "multi12cat");
$VLAN_ID = 301 if ($global_conf eq "local");


my $verbose = 1;

my %switch_human_readable;

if ($global_conf eq "trento") {
	$switch_human_readable{'02:08:02:08:00:00:00:03'} = "CN-03, cache enabled";
	$switch_human_readable{'02:08:02:08:00:00:00:01'} = "CN-01, no cache";
	$switch_human_readable{'02:00:00:00:00:00:00:01'} = "ETH-01, cache enabled";
	$switch_human_readable{'02:00:00:00:00:00:00:03'} = "ETH-03, no cache";
	$switch_human_readable{'00:10:00:00:00:00:00:01'} = "i2C-01, no cache";
	$switch_human_readable{'00:10:00:00:00:00:00:03'} = "i2c-03, cache enabled";
	$switch_human_readable{'01:00:00:00:00:00:00:ff'} = "iM-01, no cache";
}

if ($global_conf eq "local") {
	
	my $file = 'switch.conf';
	open my $info, $file or die "Could not open $file: $!";

	# each line of switch.conf has the following format
	# iC3_c|0001000000000003
	# _c indicates that the switch is cache enabled
	
	while( my $line = <$info>)  {
		chomp($line);
	        #print "$line\n";   
	        my @code = split(/\|/,$line); #Splits on "|"
	        #print "$code[0]\n";
                my $output_str = add_colon_to_datapath($code[1]);
		#print "$output_str\n";
	        my @code2 = split(/\_/, $code[0]); #Splits on "_"
	        #print "$code2[0]\n";
	        if ( scalar(@code2) == 2)   {
	        	$code2[0] .= ", cache enabled";
	        } else {
	        	$code2[0] .= ", no cache";
	        }
	        #print "$code2[0]\n";
                $switch_human_readable{$output_str} = $code2[0];
	}

	close $info;
}


#flag: compact multiple entries that only differ for in-port number in the visualization
#this is useful because flowvisor translates a request for in-port=* into a set of specific ports
#it will be modified depending on $queryfor 
#if ($queryfor eq "flowtable") $drop_rows_with_inports = 1 
#if ($queryfor eq "flowtable-raw") $drop_rows_with_inports = 0
my $drop_rows_with_inports = 1; 
#if the previous flag is 1, it tells to collect a number of different entries that only differs
#for the input port before compacting into a single line in the visualization
#the problem is that this value should be different for each switch :-(
#so for the moment, setting it to 2 does the job for all number of ports
my $number_of_different_inports = 2; 

my $controller_ip = "";
$controller_ip = "10.216.12.88" if ($global_conf eq "i2cat");
$controller_ip = "10.216.33.109" if ($global_conf eq "trento");
$controller_ip = "10.216.12.255" if ($global_conf eq "multi12cat");
$controller_ip = "127.0.0.1" if ($global_conf eq "local");


#the following is not used as the queries are for all switches
my $url = "";
my $url_flow_sw5 = "";
my $url_flow_sw4 = "";
if ($global_conf eq "i2cat") {
	$url_flow_sw5 = "wm/core/switch/00:10:00:00:00:00:00:05/flow/json";
	$url_flow_sw4 = "wm/core/switch/00:10:00:00:00:00:00:04/flow/json";
}
if ($global_conf eq "trento") {
	$url_flow_sw5 = "wm/core/switch/02:08:02:08:00:00:00:02/flow/json";
	$url_flow_sw4 = "wm/core/switch/02:08:02:08:00:00:00:01/flow/json";
}


#my $switch="00:10:00:00:00:00:00:05";

#my $url_start = "wm/core/switch/all/tagbasedfw/start"; 
#my $url_stop = "wm/core/switch/all/tagbasedfw/stop";
my $url_info = "icn/switch/all/tagbasedfw/info";


#icn_experim.cgi?tagbasedfw=notbf
my $url_notbf = "icn/switch/all/tagbasedfw/notbf";

#icn_experim.cgi?tagbasedfw=tbf
my $url_tbf = "icn/switch/all/tagbasedfw/tbf";

#icn_experim.cgi?tagbasedfw=tbff
my $url_tbff = "icn/switch/all/tagbasedfw/tbff"; 


# this hash has a switch as key and as value the array of rows of flow table [match, action]
my %hash_of_learning_rows;
my %hash_of_conet_rows;

my %hash_of_content_rows;


my @learning_rows = qw();
my @conet_rows = qw();
my $match_text_row = "";
my $action_text_row = "";
my $packets_row = 0;
my $bytes_row = 0;

my $headers="";
my $body="";
sub chunk { my ($data,$pointer)=@_; ${$pointer}.=$data; return length($data) }

my $curl = WWW::Curl::Easy->new;


my $tbfstatus = "UNKNOWN";
my $debug_status = "";

# we always either ask info or activate or deactivate the switch 05
# then we can optionally make a query to either info about switch 05 or 04 and about flow table or cached items

if ($tagbasedfw eq "") {
	$url = $url_info;
#} elsif ($tagbasedfw eq "deactivate") {
#	$url = $url_stop; 
#} elsif ($tagbasedfw eq "activate") {
#	$url = $url_start; 
} elsif ($tagbasedfw eq "notbf") {
	$url = $url_notbf; 
} elsif ($tagbasedfw eq "tbf") {
	$url = $url_tbf; 
} elsif ($tagbasedfw eq "tbff") {
	$url = $url_tbff; 
}

$curl->setopt(CURLOPT_URL, "http://$controller_ip:8080/$url");
$curl->setopt(CURLOPT_WRITEFUNCTION, \&chunk );
$curl->setopt(CURLOPT_WRITEHEADER, \$headers );
$curl->setopt(CURLOPT_FILE, \$body );

# Starts the actual first request
my $retcode = $curl->perform;
if ($verbose) {
	dbg_addrow(  "sent URL: http://$controller_ip:8080/$url" );
}

my $perl_scalar = process_results ();
if ( $perl_scalar ne "" ) {
	#print $perl_scalar, "\n";
	#foreach my $key (keys %$perl_scalar){
	#	print ">>> $key = $$perl_scalar{$key}\n";
	#}
	#print $$perl_scalar{"00:10:00:00:00:00:00:05"}, "\n";
	my $array_ref = $$perl_scalar{"all"};
	# print $array_ref , "\n";
	# print @ {$array_ref}, "\n";
	# print $ { $array_ref}[0] , "\n";
	# print $ { $array_ref}[0]->{"cacheditems"} , "\n";
	# print $array_ref->[0]->{"cacheditems"} , "\n";
	if ($verbose) {
		dbg_addrow( "Decoded value from JSON : " . $array_ref->{"tagbasedfw"} );
	}
#	if ($array_ref->{"tagbasedfw"} eq "up") {
#		$tbfstatus = "UP";
#	} elsif ($array_ref->{"tagbasedfw"} eq "down") {
#		$tbfstatus = "DOWN";
	if ($array_ref->{"tagbasedfw"} eq "NOTBF") {
		$tbfstatus = "TBF DISABLED";
	} elsif ($array_ref->{"tagbasedfw"} eq "TBF") {
		$tbfstatus = "TBF ACTIVE";
	} elsif ($array_ref->{"tagbasedfw"} eq "TBFF") {
		$tbfstatus = "TBF + FILTER";
	} else {
		#$tbfstatus = "UNKNOWN";
	}
}
		
#the second request is made if $queryfor is flowtable or cachedcontents
if ($queryfor eq "flowtable" || $queryfor eq "flowtable-raw" || $queryfor eq "cachedcontents") {

	#$url = $url_flow_sw5;

	if ($queryfor eq "flowtable" || $queryfor eq "flowtable-raw") {
		$url = "wm/core/switch/";
	} elsif ( $queryfor eq "cachedcontents") {
		$url = "icn/cache-server/";
	}	
	
	if ($querysw eq "all" || $querysw eq "" ) {
		$url .= "all/";	
	} else {
		$url .= $querysw ."/";
	}

	if ($queryfor eq "flowtable" || $queryfor eq "flowtable-raw") {
		$url .= "flow/json"
	} elsif ( $queryfor eq "cachedcontents") {
		$url .= "cachedcontents/json"
	}
	
	
	$headers="";
	$body="";
# 	my $curl = WWW::Curl::Easy->new;
	$curl->setopt(CURLOPT_URL, "http://$controller_ip:8080/$url");
	$curl->setopt(CURLOPT_WRITEFUNCTION, \&chunk );
	$curl->setopt(CURLOPT_WRITEHEADER, \$headers );
	$curl->setopt(CURLOPT_FILE, \$body );
	
	
	my $retcode = $curl->perform;
	if ($verbose) {
		dbg_addrow ("sent URL: http://$controller_ip:8080/$url");
		dbg_addrow ("queryfor:  $queryfor");
	}

	my $perl_scalar = process_results ();
	if ( $perl_scalar ne "" && ($queryfor eq "flowtable" || $queryfor eq "flowtable-raw")) {
		$drop_rows_with_inports = 1 if ($queryfor eq "flowtable"); 
		$drop_rows_with_inports = 0 if ($queryfor eq "flowtable-raw");

		#$debug_status .= $perl_scalar . "\n";
		foreach my $key (keys %$perl_scalar){
			#	$debug_status .= ">>> $key = $$perl_scalar{$key}\n";
			#} 
			#$debug_status .= $$perl_scalar{"00:10:00:00:00:00:00:05"} . "\n"; 
			#my $array_ref = $$perl_scalar{"00:10:00:00:00:00:00:05"};
			my $array_ref = $$perl_scalar{$key};
			#$debug_status .= $array_ref . "\n";
			#$debug_status .= $array_ref->[0] . "\n";
			#$debug_status .= $array_ref->[0]->{"length"}  . "\n";

			%hash_of_learning_rows->{$key} = [];
			%hash_of_conet_rows->{$key} = [];
			
			for my $flow_table_row (@$array_ref) {

				$packets_row = $flow_table_row->{'packetCount'};
				$bytes_row = $flow_table_row->{'byteCount'};

				$match_text_row = "";
				
				#debug_win ( $flow_table_row->{"length"});
				my $match = $flow_table_row->{'match'};
				#my $packetCount = $flow_table_row->{'packetCount'};
				#my $byteCount = $flow_table_row->{'byteCount'};
				
				#processing of an entry is done for our VLAN or for all VLANs 
				#it should be a problem of flowvisor, but we see entries of other VLANs
				if (!$drop_rows_with_inports || $match->{'dataLayerVirtualLan'} eq $VLAN_ID) {

					if (!$drop_rows_with_inports) {
						my $vlan_rule = $match->{'dataLayerVirtualLan'};
						add2match ("VLAN:$vlan_rule ");
					}
					
					my $priority = $flow_table_row->{'priority'};
						add2match ("prio:$priority ");
		
					
					if ($match->{'dataLayerType'} eq "0x0800") {
						add2match ("IP ");
					}
					
					if ($match->{'dataLayerType'} eq "0x0806") {
						add2match ("ARP ");
					}
		
					if ($match->{'inputPort'} != 0) {
						add2match ("inPort:".substr(("0".$match->{'inputPort'}),-2)." ");
					}
					my $dl = $match->{'dataLayerSource'};
					if ($dl ne "00:00:00:00:00:00") {
						$dl =~ s/://g;
						add2match ("DL-S:".$dl." ");
					}
		
					$dl = $match->{'dataLayerDestination'};
					if ($dl ne "00:00:00:00:00:00") {
						$dl =~ s/://g;
						add2match ("DL-D:".$dl." ");
					}
	# 				if ($match->{'dataLayerDestination'} ne "00:00:00:00:00:00") {
	# 					add2match ("DL-D:".$match->{'dataLayerDestination'}." ");
	# 				}
		
					if ($match->{'networkSource'} ne "0.0.0.0") {
						add2match ("IP-S:".$match->{'networkSource'});
						my $netmasklen=$match->{'networkSourceMaskLen'};
						if ($netmasklen==0 || $netmasklen==32) {
							#do nothing 
						} elsif ($netmasklen > 0 && $netmasklen<32) {
							add2match ("/$netmasklen");
						}
						add2match(" ");	
					}
		
					if ($match->{'networkDestination'} ne "0.0.0.0") {
						add2match ("IP-D:".$match->{'networkDestination'});
						my $netmasklen=$match->{'networkDestinationMaskLen'};
						if ($netmasklen==0 || $netmasklen==32) {
							#do nothing 
						} elsif ($netmasklen > 0 && $netmasklen<32) {
							add2match ("/$netmasklen");
						}
						add2match(" ");	
					}			
					if ($match->{'networkProtocol'} == 17) {
						add2match ("proto:17 ");
						
						if (($match->{'transportSource'} != 0) || ($match->{'transportDestination'} != 0)) {
							my $tr_src = $match->{'transportSource'};
							$tr_src += (($tr_src<0) ? 65536 : 0);
							my $tr_dst = $match->{'transportDestination'};
							$tr_dst += (($tr_dst<0) ? 65536 : 0);
							add2match ("TP-S:".$tr_src." ");
							add2match ("TP-D:".$tr_dst." ");
							add2match ("(TAG: " . ($tr_src*65536 + $tr_dst) .")");
						}
					}
		
					$action_text_row = "";
		
					my $action_arr = $flow_table_row->{'actions'};
					for my $action (@$action_arr) {
						if ($action->{'type'} eq "OUTPUT") {
							add2action ("OUTPUT (port:".substr(("0".$action->{'port'}),-2).") ") ;
						} elsif ($action->{'type'} eq "SET_DL_SRC"){
							#my $bytes = decode_base64($action->{'dataLayerAddress'});
							#my $hstring = unpack ("H*",$bytes);
							my $hstring = $action->{'dataLayerAddress'};
							$hstring =~ s/://g;
							add2action ("SET_DL-S:".$hstring." ") ;
							
						} elsif ($action->{'type'} eq "SET_DL_DST"){
							#my $bytes = decode_base64($action->{'dataLayerAddress'});
							#my $hstring = unpack ("H*",$bytes);
							my $hstring = $action->{'dataLayerAddress'};
							$hstring =~ s/://g;
							add2action ("SET_DL-D:".$hstring." ") ;
						} 
					}
	
					if ($priority < 200) {
						#add2match ("LEARNING ");
						#push (@learning_rows, [$match_text_row, $action_text_row]);
						
						if ($drop_rows_with_inports) {
							if ($queryfor eq "flowtable") { 
								ordered_insert_row (%hash_of_learning_rows->{$key}, [$match_text_row, $action_text_row]) ;
							}
							if ($queryfor eq "flowtable-raw") { 
								ordered_insert_row (%hash_of_learning_rows->{$key}, [$match_text_row, $action_text_row, $packets_row, $bytes_row]) ;
							}
						} else {
							dbg_addrow ("added match: $match_text_row");
							if ($queryfor eq "flowtable") { 
								push (@{%hash_of_learning_rows->{$key}}, [$match_text_row, $action_text_row]);
							}
							if ($queryfor eq "flowtable-raw") { 
								push (@{%hash_of_learning_rows->{$key}}, [$match_text_row, $action_text_row, $packets_row, $bytes_row]);
							}
						}
					} else {
						#add2match ("CONET ");
						if ($drop_rows_with_inports) {
							if ($queryfor eq "flowtable") { 
								ordered_insert_row (%hash_of_conet_rows->{$key}, [$match_text_row, $action_text_row]) ;
							}
							if ($queryfor eq "flowtable-raw") { 
								ordered_insert_row (%hash_of_conet_rows->{$key}, [$match_text_row, $action_text_row, $packets_row, $bytes_row]) ;
							}
						} else {
							dbg_addrow ("added match: $match_text_row");
							if ($queryfor eq "flowtable") { 
								push (@{%hash_of_conet_rows->{$key}}, [$match_text_row, $action_text_row]);
							}
							if ($queryfor eq "flowtable-raw") { 
								push (@{%hash_of_conet_rows->{$key}}, [$match_text_row, $action_text_row, $packets_row, $bytes_row]);
							}
						}
					}
					#dbg_addrow ("processata riga >> " .$match_text_row." | ".$action_text_row);
				}
			}
		}
 	}
 	
	if ( $perl_scalar ne "" && $queryfor eq "cachedcontents") {
		foreach my $key (keys %$perl_scalar){
		
			my $array_ref = $$perl_scalar{$key};
			
			#my $array_ref = $map_ref->{'0010000000000005'};
			
			%hash_of_content_rows->{$key} = [];
			
			for my $content_table_row (@$array_ref) {

				my $nid = $content_table_row->{'nid'};
				my $csn = $content_table_row->{'csn'};
				my $tag = $content_table_row->{'tag'};
	
				push (@{%hash_of_content_rows->{$key}}, [$nid, $csn, $tag]);
	
				#dbg_addrow ("processata riga >> $nid | $csn | $tag ");
			}
		}
		
	}
	
}

#we take a row and try to insert it into the list of rows
#we exclude the port number and insert it in lexicographic order 
#so we can check duplicate rows that only differ from port number
#when we have all the port number we can substitute with a port:* row
#this means that delete port:xx from the row

# sub ordered_insert_row (%hash_of_learning_rows->{$key}, [$match_text_row, $action_text_row]) ;
sub ordered_insert_row  {
	my ($row_array, $match_action) = @_;
	my $number_of_eq_rows = 0;
	my $match = $match_action->[0];
	dbg_addrow ("process match : $match");
	my $index = index($match, "inPort:");
	my $inport = substr($match,$index+7,2);
	my $match_no_port = substr ($match, 0, $index) . substr ($match,$index+7+3);
	#dbg_addrow ("in insert row match_noport : $match_no_port");

    # do something with $row_array->[$i]
	#foreach my $item (@$row_array) {
	my $added = 0;
	my $last_position = 0;
	
#	my $i=-1;
#	foreach my $item (@$row_array) {
	for (my $i = 0; $i <= $#$row_array; $i++) {
		#$i++;
		#dbg_addrow ("beep : $i");
		my $item = $row_array->[$i];
		my $item_match = $item->[0];
		my $item_index = index($item_match, "inPort:");
		my $item_inport = "";
		my $item_match_no_port ="";
		if ($item_index!=-1) {
			$item_inport = substr($item_match,$item_index+7,2);
			$item_match_no_port = substr ($item_match, 0, $item_index) . substr ($item_match,$item_index+7+3);
		} else {
			$item_inport = "*";
			$item_match_no_port = $item_match; 
		}
		my $cmp_result = ($match_no_port cmp $item_match_no_port);
		#dbg_addrow ("confronto $match_no_port\nconfronto $item_match_no_port");
		
		if ($cmp_result == -1 ) { #insert before in the array : in position $i
			if ($number_of_eq_rows == 0 ) {
				splice @$row_array, $i, 0, $match_action;
				$added=1;
				#dbg_addrow ("internal adding ");
			}
			last;
		} elsif ($cmp_result == 0 ) { #check how many we have
			if ($item_inport != $inport) {
				$number_of_eq_rows ++;
				$last_position = $i;
			} else {
				#everything is equal... will do nothing
				$added=1;
			}
		}
#		my $str2 = $item->[1];
#		dbg_addrow (">>>> $item_match : $str2 ");
    	}
    	if ($added == 0) {
    		if ($number_of_eq_rows == 0 ) {
    			#dbg_addrow ("adding match action ");
	    		push (@$row_array, $match_action);
	    	} elsif ($number_of_eq_rows < $number_of_different_inports-1) {
			#push (@$row_array, $match_action);
			#dbg_addrow ("adding match action equal to previous");
			splice @$row_array, $last_position+1, 0, $match_action;
		} else {
		#we can replace the inport: specific rows row with a row without inport: (inport:*)
			#dbg_addrow ("adding match action with wildcard");
			splice @$row_array, $last_position+2-$number_of_different_inports, $number_of_different_inports-1, [$match_no_port,$match_action->[1]] ;
		} 
    	}
    	
	
}

print "Content-type:text/html\r\n\r\n";

print '<html>';
print '<head>';
print '<title>Testbed - ICN experiment</title>';
print '</head>';
print '<body>';

print_header ();

print "<h1>$web_page_header</h1>";

print '<table class="status" border="1">';
print '  <tbody>';

print '    <tr class="statusEven">';
print '      <td class="statusEven"><b>GOTO SLICE:</b></td>';
print '      <td class="statusEven"> 
                             &nbsp;<a href="http://10.216.33.91/cgi-bin/icn_experim.cgi?target=trento">multisite</a>
                             &nbsp;<a href="http://10.216.33.91/cgi-bin/icn_experim.cgi?target=multi12cat">multisite small</a>
                             &nbsp;<a href="http://10.216.33.91/cgi-bin/icn_experim.cgi?target=i2cat">i2cat</a>
             </td>';
print '    </tr>';
print '   </tbody>';
print '</table>';

print ' &nbsp;';
print '<table class="status" border="1">';
print '  <tbody>';

print '    <tr class="statusEven">';
print '      <td class="statusEven"><b>SERVICE</b></td>';
print '      <td class="statusEven"><b>STATUS</b></td>';
print '      <td class="statusEven"><b>COMMANDS</b></td>';
print '    </tr>';

print '    <tr class="statusEven">';
print '      <td class="statusEven">Tag based forwarding</td>';
print "      <td class='statusHOSTUP'>$tbfstatus</td>";

#my $operation = "check status";
#my $operation_url = "";
print "      <td class='miniStatusOK'>";

my $target_string = "";
if (! ($target eq "")) {
	$target_string = "&target=$target";
}


if ($tbfstatus eq "UNKNOWN") {
	print "<a href='icn_experim.cgi?$target_string'>check status</a>";
	#print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=activate'>activate</a>";
	#print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=deactivate'>deactivate</a>";

	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=notbf$target_string'>disable TBF</a>";
	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=tbf$target_string'>TBF</a>";
	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=tbff$target_string'>TBF+Filter</a>";

} elsif ($tbfstatus eq "TBF DISABLED") {
	#print "<a href='icn_experim.cgi?tagbasedfw=activate'>activate</a>";
	print "&nbsp;<a href='icn_experim.cgi?$target_string'>check status</a>";

	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=tbf$target_string'>TBF</a>";
	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=tbff$target_string'>TBF+Filter</a>";

	
} elsif ($tbfstatus eq "TBF ACTIVE") {
	#print "<a href='icn_experim.cgi?tagbasedfw=activate'>activate</a>";
	print "&nbsp;<a href='icn_experim.cgi?$target_string'>check status</a>";

	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=notbf$target_string'>disable TBF</a>";
	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=tbff$target_string'>TBF+Filter</a>";

	
} elsif ($tbfstatus eq "TBF + FILTER") {
	#print "<a href='icn_experim.cgi?tagbasedfw=deactivate'>deactivate</a>";
	print "&nbsp;<a href='icn_experim.cgi?$target_string'>check status</a>";

	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=notbf$target_string'>disable TBF</a>";
	print "&nbsp;/&nbsp;<a href='icn_experim.cgi?tagbasedfw=tbf$target_string'>TBF</a>";

}
print "</td>";

print '    </tr>';
print '   </tbody>';
print '</table>';

print "<br><br><font size=+1><a href='/cgi-bin/mrtg-rrd2.cgi/controller/?$target_string'>Show real time bandwidth information</a></font>\n";

print "<br><br><font size=+1><a href='icn_experim.cgi?queryfor=flowtable$target_string'>Query flow tables of all switches</a>&nbsp;";
print "(<a href='icn_experim.cgi?queryfor=flowtable-raw$target_string'>raw list and stats</a>)</font>\n";

print "<br><br><font size=+1><a href='icn_experim.cgi?queryfor=cachedcontents$target_string'>Query list of cached items names</a></font>\n";

if ($queryfor eq "flowtable" || $queryfor eq "flowtable-raw") {

	foreach my $key (keys %hash_of_conet_rows){

		print "<br><br><br><font style='font-weight: bold;' size='+1'>Switch ID: $key";
		if( exists($switch_human_readable{$key} )) {
			print " ($switch_human_readable{$key})";
		}
		print "</font>";

		my $qt = HTML::QuickTable->new(
			#	font_face => 'helvetica',
			#	table_width => '95%',
			border => 1,
			labels => 1
		);

		if ($queryfor eq "flowtable") {
			unshift @{%hash_of_learning_rows->{$key}}, ['Match','Action'];
		}
		if ($queryfor eq "flowtable-raw") { 
			unshift @{%hash_of_learning_rows->{$key}}, ['Match','Action','Packets','Bytes'];
		}
		print "<br><br><font style='font-weight: bold;'>Flow table entries for learning switch</font>";
		print $qt->render(%hash_of_learning_rows->{$key});
	
		$qt = HTML::QuickTable->new(
		#	font_face => 'helvetica',
		#	table_width => '95%',
			border => 1,
			labels => 1
		);
#		unshift @conet_rows, ['Match','Action'];
		if ($queryfor eq "flowtable") { 
			unshift @{%hash_of_conet_rows->{$key}}, ['','Match','Action'];
		}
		if ($queryfor eq "flowtable-raw") { 
			unshift @{%hash_of_conet_rows->{$key}}, ['','Match','Action','Packets','Bytes'];
		}


		for (my $i = 1; $i < scalar @{%hash_of_conet_rows->{$key}}; $i++) {
			unshift @{$hash_of_conet_rows{$key}[$i]},  $i;
		}
		
		
		print "<br><br><font style='font-weight: bold;'>Flow table entries for tag based forwarding</font>";
#		print $qt->render(\@conet_rows);
		print $qt->render(%hash_of_conet_rows->{$key});
	
	}

}

if ($queryfor eq "cachedcontents" ) {

	foreach my $key (keys %hash_of_content_rows){

		print "<br><br><br><font style='font-weight: bold;' size='+1'>Cache server ID: $key</font>";
		unshift @{%hash_of_content_rows->{$key}}, ['','Nid','Csn','Tag'];
		for (my $i = 1; $i < scalar @{%hash_of_content_rows->{$key}}; $i++) {
			unshift @{$hash_of_content_rows{$key}[$i]},  $i;
		}

		my $qt = HTML::QuickTable->new(
			#	font_face => 'helvetica',
			#	table_width => '95%',
			border => 1,
			labels => 1
		);
	
		print "<br><br><font style='font-weight: bold;'>Names of cached items</font>";
		print $qt->render(%hash_of_content_rows->{$key});
	
		
	}
}


if ($verbose) {
	print "<br><br><br><br>Debug window<br>";
	print "<textarea NAME='debug' cols=80 rows=5  readonly='readonly'>$debug_status</textarea>";
	print "<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>";
}

# print "<br><br><br><br><font size=+1>Environment</font>\n";
# foreach (sort keys %ENV)
# {
#   print "<b>$_</b>: $ENV{$_}<br>\n";
# }



print '</body>';
print '</html>';

1;

#only works when datapath is expressed as a string of 16 chars
sub add_colon_to_datapath {
	my $input_str = $_[0];
	#print "input string: $input_str\n";
	my $output_str = "";
	my $i=0;
	for (; $i < 7; $i++) {
		$output_str .=  substr ($input_str, 2*$i, 2).":";
	}
	$output_str .=  substr ($input_str, 2*$i, 2);
	return $output_str;
}

sub dbg_addrow ($) {
	$debug_status .= $_[0] . "\n";
}

sub add2match ($) {
	$match_text_row .= $_[0] ;
}

sub add2action ($) {
	$action_text_row .= $_[0] ;
}


#it is generic for each query: it just returns a JSON result value or a ""
#further processing of the JSON result is specific for each query
sub process_results () {
	my $result_value = "";
	# Looking at the results...
	if ($retcode == 0) {
		if ($verbose) {
			dbg_addrow( "Transfer went ok (Retcode==0)");
		}
		my $response_code = $curl->getinfo(CURLINFO_HTTP_CODE);
		# judge result and next action based on $response_code
	
		if ($verbose) {
			dbg_addrow ( "Response code: $response_code");
		}
	
		if ($response_code == 200){
			
			if ($verbose) {
				dbg_addrow ( "Received response: $body");
			}
			
			$result_value = decode_json $body;
		} else {
				#$tbfstatus = "UNKNOWN";
			if ($verbose) {
				dbg_addrow ( "HTTP response code was not 200 ");
			}
		}
	} else {
		# Error code, type of error, error message
		if ($verbose) { 
			dbg_addrow ( "An error happened: $retcode ".$curl->strerror($retcode)." ".$curl->errbuf);
		}
		#$tbfstatus ="UNKNOWN";
	}
	return $result_value;	
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

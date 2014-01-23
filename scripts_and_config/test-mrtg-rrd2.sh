#!/bin/bash
# /opt/lampp/cgi-bin/test-mrtg-rrd2.sh
# for local testing of mrtg-rrd2.cgi
# wget https://www.dropbox.com/s/334529psoett16t/test-mrtg-rrd2.sh

#see http://stackoverflow.com/questions/2224158/how-can-i-send-post-and-get-data-to-a-perl-cgi-script-via-the-command-line
#see http://www.cgi101.com/book/ch3/text.html

# export QUERY_STRING="queryfor=flowtable"
export QUERY_STRING="target=trento"

export REQUEST_URI="/cgi-bin/mrtg-rrd2.cgi/controller/?target=trento"

export REQUEST_METHOD=GET
/usr/bin/perl -w mrtg-rrd2.cgi


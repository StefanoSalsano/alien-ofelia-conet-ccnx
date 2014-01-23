#!/bin/bash
# /opt/lampp/cgi-bin/test-icn_experim.sh
# for local testing of icn_experim.cgi
# wget https://www.dropbox.com/s/a0sxesfxax0so7a/test-icn_experim.sh

#see http://stackoverflow.com/questions/2224158/how-can-i-send-post-and-get-data-to-a-perl-cgi-script-via-the-command-line
#see http://www.cgi101.com/book/ch3/text.html

# export QUERY_STRING="queryfor=flowtable"
export QUERY_STRING="queryfor=cachedcontents"

export REQUEST_URI="/cgi-bin/icn_experim.cgi/?queryfor=cachedcontents"

export REQUEST_METHOD=GET
/usr/bin/perl -w icn_experim.cgi


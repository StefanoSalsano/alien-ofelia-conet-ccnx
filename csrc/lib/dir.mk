# lib/dir.mk
# 
# Part of the CCNx distribution.
#
# Copyright (C) 2009-2012 Palo Alto Research Center, Inc.
#
# This work is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 as published by the
# Free Software Foundation.
# This work is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.
#

LDLIBS = -L$(CCNLIBDIR) $(MORE_LDLIBS) -lccn
EXPATLIBS = -lexpat
CCNLIBDIR = ../lib

PROGRAMS = hashtbtest skel_decode_test \
    encodedecodetest signbenchtest basicparsetest ccnbtreetest

BROKEN_PROGRAMS =
DEBRIS = ccn_verifysig _bt_* test.keystore
CSRC = ccn_bloom.c \
       ccn_btree.c ccn_btree_content.c ccn_btree_store.c \
       ccn_buf_decoder.c ccn_buf_encoder.c ccn_bulkdata.c \
       ccn_charbuf.c ccn_client.c ccn_coding.c ccn_digest.c ccn_extend_dict.c \
       ccn_dtag_table.c ccn_indexbuf.c ccn_interest.c ccn_keystore.c \
       ccn_match.c ccn_reg_mgmt.c ccn_face_mgmt.c \
       ccn_merkle_path_asn1.c ccn_name_util.c ccn_schedule.c \
       ccn_seqwriter.c ccn_signing.c \
       ccn_sockcreate.c ccn_traverse.c ccn_uri.c \
       ccn_verifysig.c ccn_versioning.c \
       ccn_header.c \
       ccn_fetch.c \
       lned.c \
       encodedecodetest.c hashtb.c hashtbtest.c \
       signbenchtest.c skel_decode_test.c \
       basicparsetest.c ccnbtreetest.c \
       ccn_sockaddrutil.c ccn_setup_sockaddr_un.c \
	   conet/conet.c conet/conet_packet.c conet/conet_timer.c
LIBS = libccn.a
LIB_OBJS = ccn_client.o ccn_charbuf.o ccn_indexbuf.o ccn_coding.o \
       ccn_dtag_table.o ccn_schedule.o ccn_extend_dict.o \
       ccn_buf_decoder.o ccn_uri.o ccn_buf_encoder.o ccn_bloom.o \
       ccn_name_util.o ccn_face_mgmt.o ccn_reg_mgmt.o ccn_digest.o \
       ccn_interest.o ccn_keystore.o ccn_seqwriter.o ccn_signing.o \
       ccn_sockcreate.o ccn_traverse.o \
       ccn_match.o hashtb.o ccn_merkle_path_asn1.o \
       ccn_sockaddrutil.o ccn_setup_sockaddr_un.o \
       ccn_bulkdata.o ccn_versioning.o ccn_header.o ccn_fetch.o \
       ccn_btree.o ccn_btree_content.o ccn_btree_store.o \
       lned.o \
	   conet/conet.o conet/conet_packet.o conet/conet_timer.o

default all: dtag_check lib $(PROGRAMS)
# Don't try to build shared libs right now.
# all: shlib

all: ccn_verifysig

install: install_headers
install_headers:
	@test -d $(DINST_INC) || (echo $(DINST_INC) does not exist.  Please mkdir -p $(DINST_INC) if this is what you intended. && exit 2)
	mkdir -p $(DINST_INC)/ccn
	for i in `cd ../include/ccn && echo *.h`; do                \
	    cmp -s ../include/ccn/$$i $(DINST_INC)/ccn/$$i || \
	        cp ../include/ccn/$$i $(DINST_INC)/ccn/$$i || \
	        exit 1;                                             \
	done

uninstall: uninstall_headers
uninstall_headers:
	test -L $(DINST_INC)/ccn && $(RM) $(DINST_INC)/ccn ||:
	test -L $(DINST_INC) || $(RM) -r $(DINST_INC)/ccn

shlib: $(SHLIBNAME)

lib: libccn.a

test: default encodedecodetest ccnbtreetest
	./encodedecodetest -o /dev/null
	./ccnbtreetest
	./ccnbtreetest - < q.dat
	$(RM) -R _bt_*

dtag_check: _always
	@./gen_dtag_table 2>/dev/null | diff - ccn_dtag_table.c | grep '^[<]' >/dev/null && echo '*** Warning: ccn_dtag_table.c may be out of sync with tagnames.cvsdict' || :

libccn.a: $(LIB_OBJS)
	$(RM) $@
	$(AR) crus $@ $(LIB_OBJS)

shared: $(SHLIBNAME)

$(SHLIBNAME): libccn.a $(SHLIBDEPS)
	$(LD) $(SHARED_LD_FLAGS) $(OPENSSL_LIBS) -lcrypto -o $@ libccn.a

$(PROGRAMS): libccn.a

hashtbtest: hashtbtest.o
	$(CC) $(CFLAGS) -o $@ hashtbtest.o $(LDLIBS)

skel_decode_test: skel_decode_test.o
	$(CC) $(CFLAGS) -o $@ skel_decode_test.o $(LDLIBS)

basicparsetest: basicparsetest.o libccn.a
	$(CC) $(CFLAGS) -o $@ basicparsetest.o $(LDLIBS) $(OPENSSL_LIBS) -lcrypto

encodedecodetest: encodedecodetest.o
	$(CC) $(CFLAGS) -o $@ encodedecodetest.o $(LDLIBS) $(OPENSSL_LIBS) -lcrypto

ccn_digest.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_digest.c

ccn_extend_dict.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_extend_dict.c

ccn_keystore.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_keystore.c

ccn_signing.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_signing.c

ccn_sockcreate.o:
	$(CC) $(CFLAGS) -c ccn_sockcreate.c

ccn_traverse.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_traverse.c

ccn_merkle_path_asn1.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_merkle_path_asn1.c

ccn_header.o:
	$(CC) $(CFLAGS) -c ccn_header.c

ccn_fetch.o:
	$(CC) $(CFLAGS) -c ccn_fetch.c

ccn_verifysig.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c ccn_verifysig.c

ccn_verifysig: ccn_verifysig.o
	$(CC) $(CFLAGS) -o $@ ccn_verifysig.o $(LDLIBS) $(OPENSSL_LIBS) -lcrypto

signbenchtest.o:
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c signbenchtest.c

signbenchtest: signbenchtest.o
	$(CC) $(CFLAGS) -o $@ signbenchtest.o $(LDLIBS) $(OPENSSL_LIBS) -lcrypto 

ccndumppcap: ccndumppcap.o
	$(CC) $(CFLAGS) -o $@ ccndumppcap.o $(LDLIBS) $(OPENSSL_LIBS) -lcrypto -lpcap

ccnbtreetest.o:
	$(CC) $(CFLAGS) -Dccnbtreetest_main=main -c ccnbtreetest.c

ccnbtreetest: ccnbtreetest.o libccn.a
	$(CC) $(CFLAGS) -o $@ ccnbtreetest.o $(LDLIBS) $(OPENSSL_LIBS) -lcrypto

clean:
	rm -f *.o libccn.a libccn.1.$(SHEXT) $(PROGRAMS) depend
	rm -rf *.dSYM $(DEBRIS) *% *~

###############################
# Dependencies below here are checked by depend target
# but must be updated manually.
###############################
ccn_bloom.o: ccn_bloom.c ../include/ccn/bloom.h
ccn_btree.o: ccn_btree.c ../include/ccn/charbuf.h ../include/ccn/hashtb.h \
  ../include/ccn/btree.h
ccn_btree_content.o: ccn_btree_content.c ../include/ccn/btree.h \
  ../include/ccn/charbuf.h ../include/ccn/hashtb.h \
  ../include/ccn/btree_content.h ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/indexbuf.h \
  ../include/ccn/bloom.h ../include/ccn/uri.h
ccn_btree_store.o: ccn_btree_store.c ../include/ccn/btree.h \
  ../include/ccn/charbuf.h ../include/ccn/hashtb.h
ccn_buf_decoder.o: ccn_buf_decoder.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h
ccn_buf_encoder.o: ccn_buf_encoder.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/signing.h \
  ../include/ccn/ccn_private.h
ccn_bulkdata.o: ccn_bulkdata.c ../include/ccn/bloom.h \
  ../include/ccn/ccn.h ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h
ccn_charbuf.o: ccn_charbuf.c ../include/ccn/charbuf.h
ccn_client.o: ccn_client.c ../include/ccn/ccn.h ../include/ccn/coding.h \
  ../include/ccn/charbuf.h ../include/ccn/indexbuf.h \
  ../include/ccn/ccn_private.h ../include/ccn/ccnd.h \
  ../include/ccn/digest.h ../include/ccn/hashtb.h \
  ../include/ccn/reg_mgmt.h ../include/ccn/schedule.h \
  ../include/ccn/signing.h ../include/ccn/keystore.h ../include/ccn/uri.h
ccn_coding.o: ccn_coding.c ../include/ccn/coding.h
ccn_digest.o: ccn_digest.c ../include/ccn/digest.h
ccn_extend_dict.o: ccn_extend_dict.c ../include/ccn/charbuf.h \
  ../include/ccn/extend_dict.h ../include/ccn/coding.h
ccn_dtag_table.o: ccn_dtag_table.c ../include/ccn/coding.h
ccn_indexbuf.o: ccn_indexbuf.c ../include/ccn/indexbuf.h
ccn_interest.o: ccn_interest.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h
ccn_keystore.o: ccn_keystore.c ../include/ccn/keystore.h
ccn_match.o: ccn_match.c ../include/ccn/bloom.h ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/digest.h
ccn_reg_mgmt.o: ccn_reg_mgmt.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/reg_mgmt.h
ccn_face_mgmt.o: ccn_face_mgmt.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/face_mgmt.h \
  ../include/ccn/sockcreate.h
ccn_merkle_path_asn1.o: ccn_merkle_path_asn1.c \
  ../include/ccn/merklepathasn1.h
ccn_name_util.o: ccn_name_util.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/random.h
ccn_schedule.o: ccn_schedule.c ../include/ccn/schedule.h
ccn_seqwriter.o: ccn_seqwriter.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/seqwriter.h
ccn_signing.o: ccn_signing.c ../include/ccn/merklepathasn1.h \
  ../include/ccn/ccn.h ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/signing.h \
  ../include/ccn/random.h
ccn_sockcreate.o: ccn_sockcreate.c ../include/ccn/sockcreate.h
ccn_traverse.o: ccn_traverse.c ../include/ccn/bloom.h \
  ../include/ccn/ccn.h ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/uri.h
ccn_uri.o: ccn_uri.c ../include/ccn/ccn.h ../include/ccn/coding.h \
  ../include/ccn/charbuf.h ../include/ccn/indexbuf.h ../include/ccn/uri.h
ccn_verifysig.o: ccn_verifysig.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/keystore.h \
  ../include/ccn/signing.h
ccn_versioning.o: ccn_versioning.c ../include/ccn/bloom.h \
  ../include/ccn/ccn.h ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/uri.h \
  ../include/ccn/ccn_private.h
ccn_header.o: ccn_header.c ../include/ccn/ccn.h ../include/ccn/coding.h \
  ../include/ccn/charbuf.h ../include/ccn/indexbuf.h \
  ../include/ccn/header.h
ccn_fetch.o: ccn_fetch.c ../include/ccn/fetch.h ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/uri.h
lned.o: lned.c ../include/ccn/lned.h
encodedecodetest.o: encodedecodetest.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/bloom.h ../include/ccn/uri.h \
  ../include/ccn/digest.h ../include/ccn/keystore.h \
  ../include/ccn/signing.h ../include/ccn/random.h
hashtb.o: hashtb.c ../include/ccn/hashtb.h
hashtbtest.o: hashtbtest.c ../include/ccn/hashtb.h
signbenchtest.o: signbenchtest.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/keystore.h
skel_decode_test.o: skel_decode_test.c ../include/ccn/charbuf.h \
  ../include/ccn/coding.h
basicparsetest.o: basicparsetest.c ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/charbuf.h \
  ../include/ccn/indexbuf.h ../include/ccn/face_mgmt.h \
  ../include/ccn/sockcreate.h ../include/ccn/reg_mgmt.h \
  ../include/ccn/header.h
ccnbtreetest.o: ccnbtreetest.c ../include/ccn/btree.h \
  ../include/ccn/charbuf.h ../include/ccn/hashtb.h \
  ../include/ccn/btree_content.h ../include/ccn/ccn.h \
  ../include/ccn/coding.h ../include/ccn/indexbuf.h ../include/ccn/uri.h
ccn_sockaddrutil.o: ccn_sockaddrutil.c ../include/ccn/charbuf.h \
  ../include/ccn/sockaddrutil.h
ccn_setup_sockaddr_un.o: ccn_setup_sockaddr_un.c ../include/ccn/ccnd.h \
  ../include/ccn/ccn_private.h ../include/ccn/charbuf.h

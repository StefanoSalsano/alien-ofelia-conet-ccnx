# generated Linux ccnlx1 2.6.32-41-server #94-Ubuntu SMP Fri Jul 6 18:15:07 UTC 2012
#
#
#
SHEXT=so
SHLIBNAME=libccn.$(SHEXT).1
SHLIBDEPS=
SHARED_LD_FLAGS = -shared --whole-archive -soname=$(SHLIBNAME) -lc
PLATCFLAGS=-fPIC
CWARNFLAGS = -Wall -Wpointer-arith -Wreturn-type -Wstrict-prototypes
CPREFLAGS= -I../include -D_REENTRANT
COPT = -g
INSTALL_BASE = /usr/local
INSTALL_INCLUDE = $(INSTALL_BASE)/include
INSTALL_LIB = $(INSTALL_BASE)/lib
INSTALL_BIN = $(INSTALL_BASE)/bin
PCAP_PROGRAMS = ccndumppcap
RESOLV_LIBS = -lresolv
ANT = /usr/bin/ant
CP = cp
INSTALL = install
LS = /bin/ls
RM = rm -f
SH = /bin/sh
BUILD_JAVA = true
DINST_BIN = $(DESTDIR)$(INSTALL_BIN)
DINST_INC = $(DESTDIR)$(INSTALL_INCLUDE)
DINST_LIB = $(DESTDIR)$(INSTALL_LIB)

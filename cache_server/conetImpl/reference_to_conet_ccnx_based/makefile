$(making conetImpl a c implementation of conet functionalities)

#where do you want to put header files?
INCLUDE_FOLDER=../include 
#forse non si usa

#where do you want to put libconet.a?
LIB_FOLDER=../lib

CCNX_FOLDER=../../..
CCNX_INCLUDE_FOLDER=$(CCNX_FOLDER)/csrc/include
CCNX_LIB_FOLDER=$(CCNX_FOLDER)/csrc/lib

CCNX_OTHER_EX_FOLD=$(CCNX_FOLDER)/csrc/ccnd
CCNX_OTHER_EXS=$(CCNX_OTHER_EX_FOLD)/ccnd.o $(CCNX_OTHER_EX_FOLD)/ccnd_msg.o $(CCNX_OTHER_EX_FOLD)/ccnd_stats.o $(CCNX_OTHER_EX_FOLD)/ccnd_internal_client.o $(CCNX_OTHER_EX_FOLD)/ccnd_main.o


#src folder of ccnx based conet implementation
CONET_CCNX_SRC=$(CCNX_FOLDER)/csrc/lib/conet

SRC_FOLDER=../src #forse non si usa


libconetccnx.a: conet.o conet_packet.o conet_timer.o
	ar cr $(LIB_FOLDER)/libconetccnx.a $(LIB_FOLDER)/conet.o $(LIB_FOLDER)/conet_packet.o $(LIB_FOLDER)/conet_timer.o $(CCNX_OTHER_EXS)
	
#-L$(CCNX_LIB_FOLDER) -lccn -lcrypto

####################### CONET-ccnx-based implementation

	
conet.o: $(CONET_CCNX_SRC)/conet.c $(CCNX_INCLUDE_FOLDER)/conet/conet.h
	gcc -w -g -c $(CONET_CCNX_SRC)/conet.c -o $(LIB_FOLDER)/conet.o -fPIC -I$(CCNX_INCLUDE_FOLDER) -L$(CCNX_LIB_FOLDER) -lccn

conet_packet.o: $(CONET_CCNX_SRC)/conet_packet.c $(CCNX_INCLUDE_FOLDER)/conet/conet_packet.h
	gcc -w -g -c $(CONET_CCNX_SRC)/conet_packet.c -o $(LIB_FOLDER)/conet_packet.o -fPIC -I$(CCNX_INCLUDE_FOLDER) -L$(CCNX_LIB_FOLDER) -lccn
	
conet_timer.o: $(CONET_CCNX_SRC)/conet_timer.c $(CCNX_INCLUDE_FOLDER)/conet/conet_timer.h
	gcc -w -g -c $(CONET_CCNX_SRC)/conet_timer.c -o $(LIB_FOLDER)/conet_timer.o -fPIC -I$(CCNX_INCLUDE_FOLDER) -L$(CCNX_LIB_FOLDER) -lccn

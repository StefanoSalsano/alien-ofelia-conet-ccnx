$(making conetImpl a c implementation of conet functionalities)

#where do you want to put header files?
INCLUDE_FOLDER=include

#where do you want to put libconet.so?
LIB_FOLDER=lib

CCNX_FOLDER=../..
CCNX_INCLUDE_FOLDER=../../csrc/include
CCNX_LIB_FOLDER=../../csrc/lib

#src folder of ccnx based conet implementation
CONET_CCNX_SRC=../../csrc/lib/conet

SRC_FOLDER=src

listener.o: $(SRC_FOLDER)/listener.c $(SRC_FOLDER)/listener.h libconet.so sender.o
	gcc -g $(SRC_FOLDER)/listener.c    -o $(LIB_FOLDER)/listener.o  -I$(SRC_FOLDER) -I$(CCNX_INCLUDE_FOLDER) -I$(INCLUDE_FOLDER) -L$(CCNX_FOLDER)/csrc/lib/ -L$(LIB_FOLDER)/ -fPIC -lconet -lconetccnx -lccn -lcrypto


sender.o: $(SRC_FOLDER)/sender.c libconet.so
	gcc -c -g $(SRC_FOLDER)/sender.c    -o $(LIB_FOLDER)/sender.o  -I$(SRC_FOLDER) -I$(CCNX_FOLDER)/csrc/ccnd -I$(CCNX_INCLUDE_FOLDER) -I$(INCLUDE_FOLDER) -L$CCNX_FOLDER/csrc/lib/ -fPIC -lconet -lconetccnx -lccn -lcrypto





libconet.so: cacheEngine.o precache.o cache_table.o utilities.o c_json.o
	gcc -shared -fPIC -o $(LIB_FOLDER)/libconet.so $(LIB_FOLDER)/cacheEngine.o $(LIB_FOLDER)/precache.o $(LIB_FOLDER)/cache_table.o $(LIB_FOLDER)/utilities.o $(LIB_FOLDER)/c_json.o

cacheEngine.o : $(SRC_FOLDER)/cacheEngine.c $(SRC_FOLDER)/cacheEngine.h $(SRC_FOLDER)/cache_table.h
	gcc -c -g $(SRC_FOLDER)/cacheEngine.c -o $(LIB_FOLDER)/cacheEngine.o -I$(SRC_FOLDER) -I$(INCLUDE_FOLDER) -I$(CCNX_INCLUDE_FOLDER) -fPIC

precache.o: $(SRC_FOLDER)/precache.c $(SRC_FOLDER)/precache.h
	gcc -c -g $(SRC_FOLDER)/precache.c -o $(LIB_FOLDER)/precache.o -fPIC

cache_table.o: $(SRC_FOLDER)/cache_table.c $(SRC_FOLDER)/cache_table.h
	gcc -c -g $(SRC_FOLDER)/cache_table.c -o $(LIB_FOLDER)/cache_table.o -I$(CCNX_INCLUDE_FOLDER) -fPIC

utilities.o: $(SRC_FOLDER)/utilities.c $(SRC_FOLDER)/utilities.h
	gcc -c -g $(SRC_FOLDER)/utilities.c -o $(LIB_FOLDER)/utilities.o -fPIC
	
c_json.o: $(SRC_FOLDER)/c_json.c $(SRC_FOLDER)/c_json.h
	gcc -c -g $(SRC_FOLDER)/c_json.c -o $(LIB_FOLDER)/c_json.o -fPIC

clean:
	rm -f $(LIB_FOLDER)/libconet.so $(LIB_FOLDER)/cjson.o $(LIB_FOLDER)/utilities.o $(LIB_FOLDER)/cache_table.o $(LIB_FOLDER)/precache.o $(LIB_FOLDER)/cacheEngine.o $(LIB_FOLDER)/sender.o $(LIB_FOLDER)/listener.o

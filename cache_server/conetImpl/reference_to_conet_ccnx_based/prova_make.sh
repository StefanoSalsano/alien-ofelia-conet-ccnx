#where do you want to put header files?
INCLUDE_FOLDER="../include"

#where do you want to put libconet.a?
LIB_FOLDER="../lib"

CCNX_FOLDER="../../ccnx401base-cp-raw"
CCNX_INCLUDE_FOLDER="../../ccnx401base-cp-raw/csrc/include"
CCNX_LIB_FOLDER="../../ccnx401base-cp-raw/csrc/lib"
CCNX_OTHER_EX_FOLD="$CCNX_FOLDER/csrc/ccnd"
CCNX_OTHER_EXS="$CCNX_OTHER_EX_FOLD/ccnd.o $CCNX_OTHER_EX_FOLD/ccnd_msg.o $CCNX_OTHER_EX_FOLD/ccnd_stats.o $CCNX_OTHER_EX_FOLD/ccnd_internal_client.o $CCNX_OTHER_EX_FOLD/ccnd_main.o"

#src folder of ccnx based conet implementation
CONET_CCNX_SRC="../../ccnx401base-cp-raw/csrc/lib/conet"

SRC_FOLDER="../src"

gcc -w -g $CONET_CCNX_SRC/conet.c $CCNX_OTHER_EXS -o $LIB_FOLDER/conet.o -fPIC -I$CCNX_INCLUDE_FOLDER -L$CCNX_LIB_FOLDER -lccn -lcrypto



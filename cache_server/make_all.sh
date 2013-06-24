echo : ATTENZIONE RICORDATI DI SETTARE LD_LIBRARY_PATH E TROVA UNA SOLUZIONE CHE POSSA FARE A MENO DI QUESTA SCRITTA

CCNX_FOLDER=".."

#export LD_LIBRARY_PATH="/home/andrea/dati/temp/OFELIA_SVN/cnit-convergence/OFELIA/software/cacheEngine/v07/cacheEngine-remoto/Listener10.04/conetImpl/lib"


echo :
echo : -------------------- MAKING ccnx modified fo conet --------------------
echo :
cd $CCNX_FOLDER
make clean && make
cd -


echo :
echo : -------------------- MAKING conet ccnx based --------------------
echo :
#cp ccnx401base-cp-raw/csrc/lib/libccn.a conetImpl/lib
#non usare ccnx401base-cp-raw/lib/libccn.a: e' uno specchietto per le allodole

cd ./conetImpl/reference_to_conet_ccnx_based
make
cd ../..



echo :
echo : -------------------- MAKING conetImpl --------------------
echo :
cd ./conetImpl
make clean 
make
cd ..

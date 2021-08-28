#!/bin/bash

RED="\033[0;31m"
DEFAULT_COLOR="\033[0m"

clientDir=./build/client
serverDir=./build/server
sockFile=cs_sock

readDestination=test/testCache_concorrenza/readDestination/
#raccoglie i file eliminati dalla cache
cacheDestination=test/testCache_concorrenza/cacheDestination


#pulisco il contenuto di cacheDestination
rm -r $cacheDestination
mkdir $cacheDestination
#creao la directory che conterra la copia di Formaggio.txt
fileDir=test/testCache_concorrenza/file/fileCopy

if [ -d $fileDir ]; then
    rm -r $fileDir
fi

mkdir $fileDir

# copio n file nella dir
n=200
for (( i=1; i <= n; i++))
do
   fileCopy=$fileDir/test$i.txt
   touch $fileCopy
   echo -n "Gatto" > $fileCopy
done


#eseguo il client per rimpizzare i file a causa dello spazio, in bytes, insufficiente
for((i = 1; i <= n; i++))
do
$clientDir -p -f cs_sock -W $fileDir/test$i.txt -D $cacheDestination &
done

m=n
#rimuovo i file
for((i = 1; i <= n; i++))
do
$clientDir -p -f cs_sock -c $fileDir/test$i.txt &
(( m++ ))
done

for((i=0;i<m;++i)); do
    wait ${PID[i]}
done


#elimino la dir con la copia dei file
rm -r $fileDir

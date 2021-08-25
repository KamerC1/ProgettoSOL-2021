#!/bin/bash


RED="\033[0;31m"
DEFAULT_COLOR="\033[0m"

clientDir=./build/client
serverDir=./build/server
sockFile=cs_sock
configFile=test/config/test2.txt

echo -e "${RED}Avvio il server${DEFAULT_COLOR}"
$serverDir $configFile &
SERVER_PID=$!
export SERVER_PID

#preparazione configurazione server e valgrind
sleep 4


readDestination=test/test2/readDestination/
#raccoglie i file eliminati dalla cache
cacheDestination=test/test2/cacheDestination


#pulisco il contenuto di cacheDestination
rm -r $cacheDestination
mkdir $cacheDestination
#creao la directory che conterra la copia di Formaggio.txt
fileDir=test/test2/file/600kb_Files/FormaggioCopy

if [ -d $fileDir ]; then
    rm -r $fileDir
fi

mkdir $fileDir

# copio n file nella dir
n=20
for (( i=1; i <= n; i++))
do
    fileCopy=$fileDir/Formaggio$i.txt
    touch $fileCopy
    cp test/test2/file/600kb_Files/Formaggio.txt $fileCopy
done

#eseguo il client per rimpizzare i file a causa dello spazio, in bytes, insufficiente
for((i = 1; i <= n; i++))
do
$clientDir -p -f $sockFile -W $fileDir/Formaggio$i.txt -D $cacheDestination
done

#elimino tutti i file dal client
for((i = 1; i <= n; i++))
do
$clientDir -p -f $sockFile -c $fileDir/Formaggio$i.txt 
done

#creo n files vuoti
if [ -d $fileDir ]; then
    rm -r $fileDir
fi
mkdir $fileDir
n=20
for (( i=1; i <= n; i++))
do
    fileCopy=$fileDir/Formaggio$i.txt
    touch $fileCopy
done

#eseguo il client per rimpizzare i file a causa dello spazio, in files, insufficiente
$clientDir -p -f cs_sock -w $fileDir


echo -e "${RED}Lancio sengale: SIGHUP${DEFAULT_COLOR}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID

for((i=0;i<n;++i)); do
    wait ${PID[i]}
done

#elimino la dir con la copia dei file
rm -r $fileDir
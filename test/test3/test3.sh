#!/bin/bash

RED="\033[0;31m"
DEFAULT_COLOR="\033[0m"

clientDir=./build/client
serverDir=./build/server
sockFile=cs_sock
configFile=test/config/test3.txt

echo -e "${RED}Avvio il server${DEFAULT_COLOR}"
$serverDir $configFile &
SERVER_PID=$!
export SERVER_PID

#preparazione configurazione server e valgrind
sleep 4

readDestination=test/test3/readDestination/
#raccoglie i file eliminati dalla cache
cacheDestination=test/test3/cacheDestination


#pulisco il contenuto di cacheDestination
rm -r $cacheDestination
mkdir $cacheDestination
#creao la directory che conterra la copia di Formaggio.txt
fileDir=test/test3/file/CiambellaCopy

if [ -d $fileDir ]; then
    rm -r $fileDir
fi

mkdir $fileDir


secs=30                           # tempo massimo di esecuzione del server
endTime=$(( $(date +%s) + secs )) # Calcola quando terminare
n=0
i=1
while [ $(date +%s) -lt $endTime ]; do
    fileCopy=$fileDir/Ciambella$i.txt
    touch $fileCopy
    cp test/test3/file/Ciambella.txt $fileCopy

    $clientDir -f $sockFile -W $fileDir/Ciambella$i.txt -a $fileDir/Ciambella$i.txt,"CIAO" -l $fileDir/Ciambella$i.txt  -r $fileDir/Ciambella$i.txt -d $readDestination -u $fileDir/Ciambella$i.txt &
    (( n++ ))
    $clientDir -f $sockFile -R -$readDestination &
    (( n++ ))
    (( i++ ))
done

echo -e "${RED}Lancio sengale: SIGINT${DEFAULT_COLOR}"
kill -s SIGINT $SERVER_PID
wait $SERVER_PID

for((i=0;i<n;++i)); do
    wait ${PID[i]}
done

echo -e "\n${RED}ATTENZIONE: il calcolo delle statistiche potrebbe richiedere un po' di tempo${DEFAULT_COLOR}\n"

#elimino la dir con la copia dei file
rm -r $fileDir

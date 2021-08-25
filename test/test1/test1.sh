#!/bin/bash
#Nota: in questo test non viene testato l'algoritmo di rimpiazzamento (per quello c'è test2)

RED="\033[0;31m"
DEFAULT_COLOR="\033[0m"

clientDir=./build/client
serverDir=./build/server
sockFile=cs_sock
configFile=test/config/test1.txt

echo -e "${RED}Avvio il server${DEFAULT_COLOR}"
valgrind --leak-check=full $serverDir $configFile &
SERVER_PID=$!
export SERVER_PID

#preparazione configurazione server e valgrind
sleep 4


timeWait=200
readDestination=test/test1/readDestination/
cacheDestination=test/test1/cacheDestination/


echo -e "${RED}Avvio i client${DEFAULT_COLOR}"
#1. scrive file1, file2 
#2. legge file1, file2 e lo memorizza su $readDestination
#3. fa l'append su file2
#4. rimuove file1
file1=test/test1/file/carne.txt
file2=test/test1/file/directory/pane.txt
$clientDir -p -t $timeWait -f $sockFile -W $file1,$file2 -D $cacheDestination -a $file2,"Hello world" -r $file1,$file2 -d $readDestination -c $file1
((n++))

#1. scrive N file sul server da dir1
#2. legge N file dal server 
dir1=test/test1/file/
N=4
$clientDir -p -t $timeWait -f $sockFile -w $dir1,$N -D $cacheDestination -R $N -d $readDestination
((n++))

# entrambi i client ottengono e rilasciano la lock su $file2
# il test è progettato in modo tale che il secondo client si metta in coda per acquisire la lock
file2=test/test1/file/directory/pane.txt
$clientDir -p -t $timeWait -f $sockFile -l $file2 -u $file2 &
((n++))
$clientDir -p -t 2000 -f $sockFile -l $file2 -u $file2
((n++))

echo -e "${RED}Lancio sengale: SIGHUP${DEFAULT_COLOR}"
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID

for((i=0;i<n;++i)); do
    wait ${PID[i]}
done
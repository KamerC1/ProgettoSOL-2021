#!/bin/bash

if [ $# -eq 0 ] 
then
    echo "Inserire pathname del file log"
    exit 1
fi

#Stampa numero di scritture
#grep -v "/": il pattern potrebbe trovarsi all'interno di un pathname, quindi va escluso
num_writeFile=$(grep "Esito writeFile: POSITIVO" $1 | grep -v "/" | wc -l)
echo "Numero di writeFile: $num_writeFile"

num_appendFile=$(grep "Esito AppendoToFile: POSITIVO" $1 | grep -v "/" | wc -l)
echo "Numero di appendFile: $num_appendFile"
echo -e "Numero totale di scritture: $(($num_writeFile + $num_appendFile))\n"


#Stampa numero di close
num_closeFile=$(grep "Esito closeFileServer: POSITIVO" $1 | grep -v "/" | wc -l)
echo "Numero di closeFile: $num_closeFile"

num_removeClientInfo=$(grep "Esito removeClientInfo: POSITIVO" $1 | grep -v "/" | wc -l)
echo "Numero di removeClientInfo: $num_removeClientInfo"

echo -e "Numero totale di chiusure: $(($num_closeFile + $num_removeClientInfo))\n"


#Stampa numero di read
num_copyFileToDirServer=$(grep "Esito copyFileToDirServer: POSITIVO" $1 | grep -v "/" | wc -l)
num_readFile=$(grep "Esito readFile: POSITIVO" $1 | grep -v "/" | wc -l)
num_readNfiles=$(grep -Eo 'Numero file letti da readN: \w+' $1 | grep -v "/" | cut -d " " -f6 | awk '{totalreadN += $1} END {print totalreadN}')
#se non è stato trovato nessun valore ==> num_readNfiles=0
if [ -z "$num_readNfiles" ]; then
    num_readNfiles=0
fi
#Stampo numero read
echo -n "Numero totale di readFile: $(($num_readFile + $num_copyFileToDirServer))"
echo " (di cui $num_copyFileToDirServer sono stati memorizzati sul disco.)"
echo "Numero totale di readNFile: $num_readNfiles"
echo -e "Numero totale di read $(($num_readFile + $num_copyFileToDirServer + $num_readNfiles))\n"


#Stampa numero di lock
echo -n "Numero totale di lockFile: "
grep "Esito lockFileServer: POSITIVO" $1 | grep -v "/" | wc -l


#Stampa numero di openFile con O_LOCK
echo -n "Numero totale di Open-lock: "
grep "Operazione: openFile \[flags: 10\]" $1 | grep -v "/" | wc -l


#Stampa numero di file rimossi

num_totalCache=$(grep -Eo '\w+ file espulsi:' $1 | grep -v "/" | cut -d " " -f1 | awk '{totalCache += $1} END {print totalCache}')
#se non è stato trovato nessun valore ==> num_totalCache=0
if [ -z "$num_totalCache" ]; then
    num_totalCache=0
fi
echo -e "Numero totale di file espulsi: $num_totalCache\n"



#Stampa numero di bytes scritti
num_bytesWritten=$(grep -Eo 'Numero byte scritti: \w+' $1 | grep -v "/" | cut -d " " -f4 | awk '{bytesWritten += $1} END {print bytesWritten}')
#se non è stato trovato nessun valore ==> num_totalCache=0
if [ -z "$num_bytesWritten" ]; then
    num_bytesWritten=0
fi
#Stampa il numero medio bytes scritti
echo "Numero totale di file bytes scritti: $num_bytesWritten"

if [ ! $(($num_writeFile + $num_appendFile)) -eq "0" ]; then
    media_bytesWritten=$(echo scale=2\; $num_bytesWritten \/ \($num_writeFile + $num_appendFile\) | bc -l)
    echo -e "Media bytes scritti: $media_bytesWritten\n"
else
    echo ""
fi



#Stampa numero di bytes letti
num_bytesLetti=$(grep -Eo 'Numero byte letti: \w+' $1 | grep -v "/" | cut -d " " -f4 | awk '{bytesLetti += $1} END {print bytesLetti}')
#se non è stato trovato nessun valore ==> num_totalCache=0
if [ -z "$num_bytesLetti" ]; then
    num_bytesLetti=0
fi
#Stampa il numero medio e totale di bytes letti
echo "Numero totale di bytes letti: $num_bytesLetti"

if [ ! $(($num_readFile + $num_copyFileToDirServer + $num_readNfiles)) -eq "0" ]; then
    media_bytesLetti=$(echo scale=2\; $num_bytesLetti \/ \($num_readFile + $num_copyFileToDirServer + $num_readNfiles\) | bc -l)
echo -e "Media bytes letti: $media_bytesLetti\n"
else
    echo ""
fi


#Stampa numero totale di file creati
echo -n "Numero totale file creati: "
grep "O_CREATE" $1 | wc -l
echo ""

#Stampa numero di richieste per ogni thread



for i in $(grep -Eo 'Thread Worker: \w+' $1 | grep -v "/" | cut -d " " -f3 | sort -n -u); do

    echo -n "Numero di Richieste del thread $i: "
    grep "$i" $1 | wc -l

done
echo ""

maxCon=0
currentCon=0

while read -r line ; do
    #echo "Processing:              $line"
    if grep -q "connessione chiusa:" <<< "$line" ; then
        ((currentCon--))
    elif grep -q "Nuova connessione:" <<< "$line" ; then
        ((currentCon++))
        if [ "$currentCon" -gt "$maxCon" ]; then
            maxCon=$currentCon
        fi 
    fi
done < <(grep "connessione" $1 | grep -v "/") #sulla VM "done <<< $(grep "connessione" $1 | grep -v "/")" non funziona

echo "Massimo numero di connessioni contemporanee: $maxCon"
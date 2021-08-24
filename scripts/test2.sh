#!/bin/bash

readDestination=../test/test2/readDestination/
#raccoglie i file eliminati dalla cache
cacheDestination=../test/test2/cacheDestination


#pulisco il contenuto di cacheDestination
rm -r $cacheDestination
mkdir $cacheDestination
#creao la directory che conterra la copia di Formaggio.txt
fileDir=../test/test2/file/600kb_Files/FormaggioCopy

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
   cp ../test/test2/file/600kb_Files/Formaggio.txt $fileCopy
done

#eseguo il client per rimpizzare i file a causa dello spazio, in bytes, insufficiente
for((i = 1; i <= n; i++))
do
./a.out -p -f cs_sock -W $fileDir/Formaggio$i.txt -D $cacheDestination
done

#elimino tutti i file dal client
for((i = 1; i <= n; i++))
do
./a.out -p -f cs_sock -c $fileDir/Formaggio$i.txt 
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
./a.out -p -f cs_sock -w $fileDir

#mettere test per i file e i bytes

for((i=0;i<n;++i)); do
    wait ${PID[i]}
done

#elimino la dir con la copia dei file
rm -r $fileDir
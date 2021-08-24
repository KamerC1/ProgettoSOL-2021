#!/bin/bash

readDestination=../test/test3/readDestination/
#raccoglie i file eliminati dalla cache
cacheDestination=../test/test3/cacheDestination


#pulisco il contenuto di cacheDestination
rm -r $cacheDestination
mkdir $cacheDestination
#creao la directory che conterra la copia di Formaggio.txt
fileDir=../test/test3/file/CiambellaCopy

if [ -d $fileDir ]; then
    rm -r $fileDir
fi

mkdir $fileDir

# copio n file nella dir
#n=50
#for (( i=1; i <= n; i++))
#do
#    fileCopy=$fileDir/Ciambella$i.txt
#    touch $fileCopy
#    cp ../test/test3/file/Ciambella.txt $fileCopy
#done

#eseguo il client per rimpizzare i file a causa dello spazio, in bytes, insufficiente
#for((i = 1; i <= n; i++))
#do
#./a.out -p -t 0 -f cs_sock -W $fileDir/Ciambella$i.txt -a $fileDir/Ciambella$i.txt,"CIAO" -l $fileDir/Ciambella$i.txt  -r $fileDir/Ciambella$i.txt -d $readDestination -c $fileDir/Ciambella$i.txt &
#done


secs=10                         # Set interval (duration) in seconds.
endTime=$(( $(date +%s) + secs )) # Calculate end time.
n=0
i=1
while [ $(date +%s) -lt $endTime ]; do  # Loop until interval has elapsed.
    fileCopy=$fileDir/Ciambella$i.txt
    touch $fileCopy
    cp ../test/test3/file/Ciambella.txt $fileCopy

    ./a.out -f cs_sock -W $fileDir/Ciambella$i.txt -a $fileDir/Ciambella$i.txt,"CIAO" -l $fileDir/Ciambella$i.txt  -r $fileDir/Ciambella$i.txt -d $readDestination -c $fileDir/Ciambella$i.txt &
    (( n++ ))
    (( i++ ))
done

echo "FINITOOOOOOOOOOOOOOOOOOOOOOOOOOTTTTTTTTTTTTTTTTTTTTOOoo"

for((i=0;i<n;++i)); do
    wait ${PID[i]}
done

#elimino la dir con la copia dei file
#rm -r $fileDir

#!/bin/bash

#spawna $2 client che fanno $2 lock e $2 unlock



if [ $# -ne 2 ]; then
    echo "passare nome del file di output e numero di client da evocare"
    exit -1
fi


OUT=$1
#OUT="/dev/null"

n=$2
file=../test/temp/file.txt
touch $file
./a.out -p -f cs_sock -a $file,"Ciao" -D ../test/readN >&      $OUT &



for((k=0;k<n;++k)); do
    r=$(( $RANDOM % 100 + 200 ))
    ./a.out -p -t $r -f cs_sock -l $file >&      $OUT &
    PID[k]=$!
done

for((k=0;k<n;++k)); do
    r=$(( $RANDOM % 100 + 200 ))    
    ./a.out -p -t $r -f cs_sock -u $file >&      $OUT &
    PID[k]=$!
done


for((i=0;i<n;++i)); do
    wait ${PID[i]}
done


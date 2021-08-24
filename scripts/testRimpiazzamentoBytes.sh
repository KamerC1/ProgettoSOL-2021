#!/bin/bash

if [ $# -ne 2 ]; then
    echo "passare nome del file di output e numero di client da evocare"
    exit -1
fi


OUT=$1
#OUT="/dev/null"

n=$2
for((k=0;k<n;++k)); do
    touch ../test/temp/$k
    ./a.out -p -f cs_sock -a ../test/temp/$k,"Ciao" -D ../test/readN >&      $OUT &
    PID[k]=$!
    rm ../test/temp/$k
done


for((i=0;i<k;++i)); do
    wait ${PID[i]}
done


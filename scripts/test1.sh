#!/bin/bash
#Nota: in questo test non viene testato l'algoritmo di rimpiazzamento (per quello c'è test2)


timeWait=200
readDestination=../test/test1/readDestination/
cacheDestination=../test/test1/cacheDestination/


#1. scrive file1, file2 
#2. legge file1, file2 e lo memorizza su $readDestination
#3. fa l'append su file2
#4. rimuove file1
file1=../test/test1/file/carne.txt
file2=../test/test1/file/directory/pane.txt
valgrind --leak-check=full ./a.out -p -t $timeWait -f cs_sock -W $file1,$file2 -D $cacheDestination -a $file2,"Hello world" -r $file1,$file2 -d $readDestination -c $file1
((n++))

#1. scrive N file sul server da dir1
#2. legge N file dal server 
dir1=../test/test1/file/
N=4
valgrind --leak-check=full ./a.out -p -t $timeWait -f cs_sock -w $dir1,$N -D $cacheDestination -R $N -d $readDestination
((n++))

# entrambi i client ottengono e rilasciano la lock su $file2
# il test è progettato in modo tale che il secondo client si metta in coda per acquisire la lock
file2=../test/test1/file/directory/pane.txt
valgrind --leak-check=full ./a.out -p -t $timeWait -f cs_sock -l $file2 -u $file2 &
((n++))
valgrind --leak-check=full ./a.out -p -t 2000 -f cs_sock -l $file2 -u $file2
((n++))


for((i=0;i<n;++i)); do
    wait ${PID[i]}
done
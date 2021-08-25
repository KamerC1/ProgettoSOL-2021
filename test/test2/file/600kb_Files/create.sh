#!/bin/bash


for (( i=1; i <= 30; i++))
do
   touch Formaggio$i.txt
   cp Formaggio.txt Formaggio$i.txt
done

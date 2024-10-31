#! /bin/bash

#this script can assemble a SQUARE matrix
#blocks should have names I_J.dat
list=`ls $1*.dat` #get all matrix pieces
l=(${list})
NN=${#l[@]}
N=`awk "BEGIN {printf \"%d\", int(sqrt(${NN}))}"` #determine matrix size

echo ${N} #if run from console, nice to know dimension is correct

for i in `seq ${N}`;do
    paste ${l[@]:$((((${i}-1))*${N})):${N}} > ${i}.txt #assemble all pieces horizontally, syntax: array[start:length_of_subarray]
    cat ${i}.txt >> $1.txt #assemble pieces vertically
    rm ${i}.txt #remove intermidiate files
done


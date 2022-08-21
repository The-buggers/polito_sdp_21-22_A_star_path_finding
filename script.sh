#!/bin/bash

make clean
make target

array=(2 3 5 7 10 15 20 50 100 200 500 1000)

for j in {0..11}
do
    for i in {1..10}
    do
        ./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA 0 103585 ${array[$j]} h >> outFAstar.txt
    done
done
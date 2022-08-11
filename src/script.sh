#!/bin/bash

make clean
make target

for i in {1..10}
do
    ./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY 321269 263446 2 h
done
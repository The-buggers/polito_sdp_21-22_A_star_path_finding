#!/bin/bash

make clean
make target

for i in {1..20}
do
    ./build/graphtest ./../Benchmark/binaryd-random-V100 0 100 7 h
done
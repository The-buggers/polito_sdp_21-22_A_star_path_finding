#!/bin/bash

make clean
make target

for i in {1}
do
    ./build/graphtest ./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA 14130775 810300 2 h
    #./build/graphtest ./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dW 1523755 1953083 2 h
    #./build/graphtest ./../Benchmark/binaryd-random-V100 0 1 2 h
    #./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY 321269 263446 10 h
    #./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA 0 103585 9 h
done
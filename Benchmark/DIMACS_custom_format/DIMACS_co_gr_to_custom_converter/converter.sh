#!/bin/bash
gcc -Wall converter.c -o converter
./converter 1070376 2712798 USA-road-d.FLA.co USA-road-d.FLA.gr ../USA-road-d.FLA.txt
rm converter
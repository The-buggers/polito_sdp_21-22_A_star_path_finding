#!/bin/bash
gcc asciitobinary.c -o asciitobinary
./asciitobinary ../DIMACS_custom_format/USA-road-d.FLA.txt ../DIMACS_custom_format/binarydUSA-road-dFLA
rm asciitobinary
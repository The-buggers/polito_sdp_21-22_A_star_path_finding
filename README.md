# polito_sdp_21-22_A_star_path_finding

## Random Graph Generation
1. cd RandomGraphGenerator
2. gcc -Wall RandomGraphGenerator.c -o randgen -lm
3. ./randgen \[Path\] \[mode 0/1 \] \[max_x\] \[max_y\] \[V\] \[max_k\]

## HPC Jupyter PoliTO
1. cd ~/ParallelAstarProject/polito_sdp_21-22_A_star_path_finding/src 
2. Check that the path of each statistics file is : "../stats/name" (in VM it is only "./stats/name" because src is the root)
3. make clean; make target
4. cd build
5. ./graphtest ../../Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY 321269 263446 2 h
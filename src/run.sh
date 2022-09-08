#!/bin/bash
################################# Launch Program ##################################################################################################################
# | input_file | read_type | [read algo info: (pn, pe, tn, te) OR tr] | algo_type | [threads algo] | source | dest | heuristic                                    # 
###################################################################################################################################################################
# ./build/graphtest $input_file 2 2 'seq' 321269 263446 'h'      # READ 2(2 threads) - Sequential A*                                                              #
# ./build/graphtest $input_file 2 2 'dijkstra' 321269 263446 'h' # READ 2(2 threads) - Sequential Dijkstra                                                        #
# ./build/graphtest $input_file 2 2 'fa' 2 321269 263446 'h'     # READ 2(2 threads) - Parallel A* - FA(2 threads)                                                #
# ./build/graphtest $input_file 2 2 'sf' 2 321269 263446 'h'     # READ 2(2 threads) - Parallel A* - HDA* - SF(2 threads)                                         # 
# ./build/graphtest $input_file 2 2 'sf2' 2 321269 263446 'h'    # READ 2(2 threads) - Parallel A* - HDA* - SFv2(2 threads)                                       #
# ./build/graphtest $input_file 2 2 'b' 2 321269 263446 'h'      # READ 2(2 threads) - Parallel A* - HDA* - B(2 threads)                                          #
# ./build/graphtest $input_file 2 2 'mp' 2 321269 263446 'h'     # READ 2(2 threads) - Parallel A* - HDA* - MP(2 threads)                                         #
# ./build/graphtest $input_file 4 2 'pnba' 321269 263446 'h'     # READ 4(2 threads) - Parallel A* - PNBA*                                                        #
###################################################################################################################################################################

################################# Input Files ##################### #  Source  #   Dest  #
# ./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA # 14130775 # 810300  #  
# ./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dW   # 1523755  # 1953083 #
# ./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY # 321269   # 263446  #
# ./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA # 0        # 103585  #
# ./../../Bench/Benchmark/binaryd-random-V100                       # 0        # 1       #
##########################################################################################

################################# Algorithms ########################################
# seq: Sequential A*                                                                #
# dijkstra: Sequential Dijkstra                                                     #
# fa: Parallel First Attempt A*                                                     #
# sf: HDA* SAS SF                                                                   #
# b: HDA* SAS B                                                                     #
# sf2: HDA* SAS SF-V2                                                               #
# mp: HDA* Message Passing [Only for Random Graph 101 nodes]                        #
# pnba: PNBA*                                                                       #
#####################################################################################

################################# Heuristics ########################################
# h: Great Circle Distance using Haversine Formula                                  #
#####################################################################################

make clean
make target
clear
input_file='./../../Bench/Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY'
for i in {1}
do
    ./build/graphtest $input_file 3 2 2 2 2 'sf2' 2 321269 263446 'h'    # READ 3(2 2 2 2) - Parallel A* - HDA* - SFv2(2 threads)  
done
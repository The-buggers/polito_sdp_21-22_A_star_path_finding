# A star path finding

<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a name="readme-top"></a>

<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About The Project</a></li>
    <li><a href="#getting-started">Getting Started</a></li>
    <li><a href="#usage">Usage</a></li>
    <li>
        <a href="#folders-content">Folders' Content</a>
        <ul>
            <li><a href="#astarengine">AstarEngine</a></li>
            <li><a href="#dijkstraengine">DijkstraEngine</a></li>
            <li><a href="#graph">Graph</a></li>
        </ul>
    </li>
    <li><a href="#files-content">Files' Content</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

A* is a graph-traversal and path-search algorithm. It is used in many contexts of computer science and not only.
It can be considered as a general case of the Dijkstra algorithm. It is a Greedy-best-first-search algorithm that uses an heuristic function to guide itself.
What it does is combining:
- Dijkstra approach: favoring nodes closed to the starting point(source)
- Greedy-best-first-search approach: favoring nodes closed to the final point(destination)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

The commands to run the entire project are in `run.sh` file.

### Installation

There are three fundamental commands: 

1. Delete previous object files 
```sh
make clear
```
2. Compile code
```sh
make target
```
3. Launch program
```sh
./build/graphtest input_file read_type read_threads algo_type [algo_threads] source dest heuristic
```
#### Input file
- Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA (benchmark path 14130775 - 810300)
- Benchmark/DIMACS_custom_format/binarydUSA-road-dW (benchmark path 1523755 - 1953083)
- Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA (benchmark path 0 - 103585)
- Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY (benchmark path 321269 - 263446)

#### Read type
- 1: Parallel read 1 (with shared memory)
- 2: Parallel read 2 (read of graph and its inverse)

#### Algorithms
- "seq": Sequential A*
- "dijk": Sequential Dijkstra
- "fa": Parallel First Attempt A*
- "sf": HDA* SAS SF
- "b": HDA* SAS B
- "mpsm": HDA* MP-SM
- "mpmq": HDA* MP-MQ
- "pnba": PNBA*

#### Heuristic
- "h": Great Circle Distance using Haversine Formula

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage

```sh
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA 1 2 'seq' 14130775 810300 'h'  # Read 2(2 threads) - Sequential A*
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dW 1 4 'dijk' 1523755 1953083 'h'   # Read 2(2 threads) - Sequential Dijkstra
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA 2 2 'fa' 2 14130775 810300 'h' # Read 2(2 threads) - Parallel A* - FA(2 threads)
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dW 2 4 'sf' 2 1523755 1953083 'h'   # Read 2(2 threads) - Parallel A* - HDA* - SF(2 threads)
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA 1 5 'mpsm' 2 0 103585 'h'       # Read 2(2 threads) - Parallel A* - HDA* - SFv2(2 threads)
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA 3 2 'b' 2 0 103585 'h'         # Read 2(2 threads) - Parallel A* - HDA* - B(2 threads)
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-BAY 3 3 'mpmq' 2 321269 263446 'h'    # Read 2(2 threads) - Parallel A* - HDA* - MP(2 threads)
src/build/graphtest Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA 2 7 'pnba' 14130775 810300 'h' # Read 4(2 threads) - Parallel A* - PNBA*
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Folders' Content -->
## Folders' Content
There are three main folders.

### AstarEngine
It contains all the A* functions and algorithms.

- Astar_ab_ba.c
- Astar_sas_b.c
- Astar_hda_mp_mq.c
- Astar_hda_mp_sf.c
- Astar_sas_sf.c
- Astar_seq.c
- Astar.h

### DijkstraEngine
It contains all the Dijkstra functions and algorithms.

- Dijkstra_seq.c
- Dijkstra.h

### Graph
It contains both functions related to priority queue and graphs.

- Graph.c
- Graph.h
- Position.c
- Position.h
- PQ.c
- PQ.h
- ST.c
- ST.h

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Files' Content -->
## Files' Content

#### Astar_ab_ba.c

It contains the main function ```ASTARshortest_path_ab_ba``` responsible to create threads and to call function ```nba```. This function implements Parallel New Bidirectional A* algorithm with two threads that take two "opposite" graphs as input, where "opposite" means with reversed edges. In this algorithm threads setup their own data structure and we use a barrier to mantain the synchronization, we also use a spin lock to guarantee mutual exclusion for the only one shared and writable variable _L_. A closed set and _PQchange_ function are also used to speed-up the algorithms and to avoid useless expansion of nodes alredy in the queue or already analyzed. 

#### Astar_sas_b.c

It contains the main function ```ASTARshortest_path_sas_b``` responsible to create threads and to call function ```sas_b```. This function implements Hash DIstributed A* algorithm with barrier termination. Synchronization is implemented with two arrays of mutexes, one _num\_threads_ and the other _V_ long. In this algorithm we implement a double barrier with a proper mutex, a counter and a semaphore, the algorithm stops when all threads hit the barrier twice and they have empty priority queue. 

#### Astar_sas_sf.c

It contains the main function ```ASTARshortest_path_sas_sf``` responsible to create threads and to call function ```sas_sf```. This function implements Hash DIstributed A* algorithm with sum flags termination. Synchronization is implemented with two arrays of mutexes, one _num\_threads_ and the other _V_ long. For the termination condition we implement sum flags method, every time a thread has an empty priority queue it sets _open\_set\_empty_ and it checks if also the other threads done it, if yes it stops.

#### Astar_hda_mp_mq.c

It contains the main function ```ASTARshortest_path_hda_mp_mq``` responsible to create threads and to call function ```hda_mp_mq```. This function implements Hash DIstributed A* algorithm with message passing with message queue as structure. In this algorithm we implement a double barrier with a proper mutex, a counter and a semaphore, the algorithm stops when all threads hit the barrier twice and they have empty message queue.

#### Astar_hda_mp_sm.c

It contains the main function ```ASTARshortest_path_hda_mp_sf```  responsible to create threads and to call function ```hda_mp_sf```. This function implements Hash DIstributed A* algorithm with sum flags termination. This algorithm implements message passing through shared memory, 1GB of memory of the system is allocated and shared with threads, through the readers and writers paradigm every thread can write with global pointer to memory and read with a local one only the node of interest (based on hashing). For the termination condition we implement sum flags method, every time a thread has an empty priority queue it sets _open\_set\_empty_ and it checks if also the other threads done it, if yes it stops.

#### Astar_seq.c

It contains the main function ```ASTARshortest_path_sequential``` that implements the sequential A* algorithm.

#### Dijkstra_seq.c

It contains the main function ```DIJKSTRA_shortest_path_sequential``` that implements the sequential Dijkstra algorithm.

#### Graph.c

It contains all the functions to load a graph in different ways such ```GRAPHload_sequential```, ```GRAPHload_parallel1```, ```GRAPHload_parallel2``` and ```GRAPHload_parallel3```. There are also different functions to work on graph like ```GRAPHinsertE``` to insert an edge and ```LINKget_wt``` to get the weight of a link. Graph need a symbol table and a linked list, and struct like _node_ or _graph_ are first-class ADT, so they completly hide their content to other files. 

#### PQ.c

It contains functions to operate on priority queue such ```PQinsert``` to insert a new element, ```PQinsert``` to change its priority, ```PQextractMin``` to extract the element with lowest priority and ```PQsearch``` to check if an element is already present.  Struct _pqueue_ is first-class ADT, so it completly hides its content to other files.

#### ST.c

It contains functions to operate on symbol table such ```STinsert``` and ```STsearch``` to respectively insert and search on a symbol table.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the Creative Commons. See `LICENSE` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Lorenzo Ippolito - [Linkedin](https://www.linkedin.com/in/lorenzo-ippolito-72312b16a) - lorenzoippolito.99@gmail.com

Mattia Rosso - [Linkedin](https://www.linkedin.com/in/mattia-rosso) - mattiarosso99@gmail.com

Fabio Orazio Mirto - [Linkedin](https://www.linkedin.com/in/fabio-orazio-mirto-359b53182) - fabioorazio.mirto@gmail.com

<p align="right">(<a href="#readme-top">back to top</a>)</p>

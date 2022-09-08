# polito_sdp_21-22_A_star_path_finding

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
    <li><a href="#acknowledgments">Acknowledgments</a></li>
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
./build/graphtest [input_file] [read_type] [threads read] [algo_type] [threads algo] [source] [dest] [heuristic]
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage

```sh
./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dUSA 14130775 810300
./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dW 1523755 1953083
./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dBAY 321269 263446
./build/graphtest ./../Benchmark/DIMACS_custom_format/binarydUSA-road-dFLA 0 103585
./build/graphtest ./../Benchmark/binaryd-random-V100 0 1
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Folders' Content -->
## Folders' Content
There are three main folders.

### AstarEngine
It contains all the A* functions and algorithms.

- Astar_ab_ba.c
- Astar_sas_b.c
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

It contains the main function ```ASTARshortest_path_ab_ba``` responsible to create threads and to call function ```nba```. This function implements Parallel New Bidirectional A* algorithm.

#### Astar_sas_b.c

It contains the main function ```ASTARshortest_path_sas_b``` responsible to create threads and to call function ```hba```. This function implements Hash DIstributed A* algorithm with barrier termination.

#### Astar_sas_sf.c

It contains the main function ```ASTARshortest_path_sas_sf```  responsible to create threads and to call function ```hba```. This function implements Hash DIstributed A* algorithm with sum flags termination.

#### Astar_seq.c

It contains the main function ```ASTARshortest_path_sequential``` that implements the sequential A* algorithm.

#### Dijkstra_seq.c

It contains the main function ```DIJKSTRA_shortest_path_sequential``` that implements the sequential Dijkstra algorithm.

#### Graph.c

It contains all the functions to load a graph in different ways such ```GRAPHload_sequential```, ```GRAPHload_parallel1```, ```GRAPHload_parallel2``` and ```GRAPHload_parallel3```. There are also different functions to work on graph like ```GRAPHinsertE``` to insert an edge and ```LINKget_wt``` to get the weight of a link.

#### PQ.c

It contains functions to operate on priority queue such ```PQinsert``` to insert a new element, ```PQinsert``` to change its priority, ```PQextractMin``` to extract the element with lowest priority and ```PQsearch``` to check if an element is already present. 

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

<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Use this space to list resources you find helpful and would like to give credit to. I've included a few of my favorites to kick things off!

* [Choose an Open Source License](https://choosealicense.com)
* [GitHub Emoji Cheat Sheet](https://www.webpagefx.com/tools/emoji-cheat-sheet)
* [Malven's Flexbox Cheatsheet](https://flexbox.malven.co/)
* [Malven's Grid Cheatsheet](https://grid.malven.co/)
* [Img Shields](https://shields.io)
* [GitHub Pages](https://pages.github.com)
* [Font Awesome](https://fontawesome.com)
* [React Icons](https://react-icons.github.io/react-icons/search)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

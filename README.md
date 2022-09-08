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

There are many great README templates available on GitHub; however, I didn't find one that really suited my needs so I created this enhanced one. I want to create a README template so amazing that it'll be the last one you ever need -- I think this is it.

Here's why:
* Your time should be focused on creating something amazing. A project that solves a problem and helps others
* You shouldn't be doing the same tasks over and over like creating a README from scratch
* You should implement DRY principles to the rest of your life :smile:

Of course, no one template will serve all projects since your needs may be different. So I'll be adding more in the near future. You may also suggest changes by forking this repo and creating a pull request or opening an issue. Thanks to all the people have contributed to expanding this template!

Use the `BLANK_README.md` to get started.

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

### AstarEngine

- Astar_ab_ba.c
- Astar_sas_b.c
- Astar_sas_sf.c
- Astar_seq.c
- Astar.h

### DijkstraEngine

- Dijkstra_seq.c
- Dijkstra.h

### Graph

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

- Astar_ab_ba.c

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the Creative Commons. See `LICENSE` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Your Name - [@your_twitter](https://twitter.com/your_username) - email@example.com

Project Link: [https://github.com/your_username/repo_name](https://github.com/your_username/repo_name)

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
#!/usr/bin/python
import networkx as nx
import numpy as np
import matplotlib.pyplot as plt
import sys
import time
import folium
from folium.plugins import MarkerCluster
import pandas as pd

def haversin_dist(a, b):
    (x1, y1) = nodes[a]
    (x2, y2) = nodes[b]
    lat_1 = y1
    lon_1 = x1
    lat_2 = y2
    lon_2 = x2

    earth_radius = 6371*1000
    phi_1 = lat_1 * np.pi/180
    phi_2 = lat_2 * np.pi/180
    delta_phi = (lat_2-lat_1) * np.pi/180
    delta_delta = (lon_2-lon_1) * np.pi/180

    a = pow(np.sin(delta_phi/2),2) + np.cos(phi_1)*np.cos(phi_2)*pow(np.sin(delta_delta/2),2)
    c = 2*np.arctan2(np.sqrt(a), np.sqrt(1-a))
    return earth_radius * c

def compute_heuristic(r, source, dest):
    h = {}
    for node in r.nodes:
        h[node] = '%d | h=%.1f'%(node,round(haversin_dist(node, dest), 1))
    return 0

def read_input(path):
    n_line = 0
    with open(path) as file:
        node_position = {}
        edges = []
        for line in file:
            if n_line == 0: # first line
                n_nodes = int(line)
            elif n_line >=1 and n_line <= n_nodes: # nodes lines
                node_id, x, y = line.split()[0:3]
                node_position[int(node_id)] = (float(x),float(y))
            else: # edges
                a, b, wt = line.split()[0:3]
                edges.append((int(a),int(b),float(wt)))
            n_line += 1
    return node_position, edges

def astar_path_wrapper(nodes, edges, source, dest, printg=True):
    r = nx.DiGraph()
    if source not in nodes or dest not in nodes:
        print('Not valide source/dest')
        return
              
    for node in nodes.keys():
        r.add_node(node, pos=nodes[node])     
    r.add_weighted_edges_from(edges)
    
    if printg:
        plt.figure(figsize=(15,15))
        pos = nx.get_node_attributes(r,'pos')
        labels = nx.get_edge_attributes(r,'weight')
        nx.draw_networkx(r, pos, arrows=True, labels=compute_heuristic(r, source, dest))
        nx.draw_networkx_edge_labels(r,pos,edge_labels=labels)
        plt.show()
    try:
        path = nx.astar_path(r, source, dest, heuristic=haversin_dist, weight='weight')
        return path
    except:
        print('No path path available from %d to %d'%(source, dest))
        
def all_simple_paths_wrapper(nodes, edges, source, dest):
    r = nx.DiGraph()
    if source not in nodes or dest not in nodes:
        print('Not valide source/dest')
        return
              
    for node in nodes.keys():
        r.add_node(node, pos=nodes[node])     
    r.add_weighted_edges_from(edges)
    path = nx.all_simple_paths(r, source, dest)
    print(list(path))
    
def read_stat_file(stat_file_path):
    visited_nodes = []
    with open(stat_file_path) as file:
        for line in file:
            visited_nodes.append(int(line))
    return visited_nodes

if __name__ == "__main__":
    file = sys.argv[1]
    source = int(sys.argv[2])
    dest = int(sys.argv[3])
    nodes, edges = read_input(file)
    path = astar_path_wrapper(nodes, edges, source, dest, printg=False)
    f = open("astar_nodes_FLA.csv", "w+")
    f.write("lat, lon\n")
    visited_nodes = read_stat_file("../src/stats/stat_astar_seq.txt")
    for node in visited_nodes:
        lat, lon = nodes[node][1], nodes[node][0]
        f.write("%f, %f\n"%(lat, lon))
    f = open("dijkstra_nodes_FLA.csv", "w+")
    f.write("lat, lon\n")
    visited_nodes = read_stat_file("../src/stats/stat_dijkstra.txt")
    for node in visited_nodes:
        lat, lon = nodes[node][1], nodes[node][0]
        f.write("%f, %f\n"%(lat, lon))
    f = open("FLA_best_path.csv", "w+")
    f.write("lat, lon\n")
    for node in path:
        lat, lon = nodes[node][1], nodes[node][0]
        f.write("%f, %f\n"%(lat, lon))
    
    
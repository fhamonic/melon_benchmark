#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

#include "input_file.hpp"

template <typename _Value>
auto parse_adj_list_dimacs(const std::filesystem::path & file_name) {
    using graph_t =
        boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                              boost::no_property,
                              boost::property<boost::edge_weight_t, _Value>>;
    using vertex_descriptor = boost::graph_traits<graph_t>::vertex_descriptor;
    using edge_descriptor = boost::graph_traits<graph_t>::edge_descriptor;
    using Edge = std::pair<int, int>;

    graph_t graph;
    typename boost::property_map<graph_t, boost::edge_weight_t>::type
        length_map;

    int nb_nodes = 0;
    std::vector<std::tuple<int, int, _Value>> arcs;

    auto gr_file = open_input_file(file_name);
    std::string line;
    while(getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(iss >> ch) {
            switch(ch) {
                case 'c':
                    break;
                case 'p': {
                    std::string format;
                    int nb_arcs = 0;
                    iss >> format >> nb_nodes >> nb_arcs;
                    break;
                }
                case 'a': {
                    int from, to;
                    _Value length;
                    if(iss >> from >> to >> length) {
                        arcs.emplace_back(from - 1, to - 1, length);
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name << std::endl;
                    std::abort();
            }
        }
    }

    graph = graph_t(nb_nodes);
    length_map = get(edge_weight, graph);
    for(const auto & [u, v, length] : arcs) {
        edge_descriptor e;
        bool inserted;
        tie(e, inserted) = add_edge(u, v, graph);
        length_map[e] = length;
    }

    return std::make_pair(graph, length_map);
}

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graph_traits.hpp>

template <typename _Value>
struct Edge_Cost {
    _Value weight;
    Edge_Cost() {}
    Edge_Cost(_Value w) : weight(w) {}
};

template <typename _Value>
auto parse_csr_dimacs(const std::filesystem::path & file_name) {
    using graph_t =
        boost::compressed_sparse_row_graph<boost::directedS, boost::no_property,
                                           Edge_Cost<_Value>>;
    using vertex_descriptor = boost::graph_traits<graph_t>::vertex_descriptor;
    using edge_descriptor = boost::graph_traits<graph_t>::edge_descriptor;
    using Edge = std::pair<int, int>;

    graph_t graph;
    std::vector<Edge_Cost<_Value>> weights;

    int nb_nodes = 0;
    int nb_arcs;
    std::vector<std::pair<int, int>> arcs;
    weights.resize(0);

    auto gr_file = open_input_file(file_name);
    std::string line;
    while(getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(iss >> ch) {
            switch(ch) {
                case 'c':
                    break;
                case 'p': {
                    std::string format;
                    iss >> format >> nb_nodes >> nb_arcs;
                    break;
                }
                case 'a': {
                    int from, to;
                    _Value length;  // was hardcoded `double`, truncating on
                                    // the way into an int-weighted graph
                    if(iss >> from >> to >> length) {
                        arcs.emplace_back(from - 1, to - 1);
                        weights.emplace_back(length);
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name << std::endl;
                    std::abort();
            }
        }
    }

    graph = graph_t(boost::edges_are_unsorted_multi_pass, arcs.begin(),
                    arcs.end(), weights.data(), nb_nodes);

    return std::make_pair(graph, weights);
}
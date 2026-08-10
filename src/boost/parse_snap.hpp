#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

auto parse_adj_list_snap(const std::filesystem::path & file_name) {
    using graph_t =
        boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                              boost::no_property, boost::no_property>;

    std::ifstream gr_file(file_name);

    int nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;
    graph_t graph(nb_nodes);

    int from, to;
    while(gr_file >> from >> to) {
        add_edge(from, to, graph);
    }

    return graph;
}

#include <boost/graph/compressed_sparse_row_graph.hpp>

auto parse_csr_snap(const std::filesystem::path & file_name) {
    using graph_t =
        boost::compressed_sparse_row_graph<boost::directedS, boost::no_property,
                                           boost::no_property>;

    std::vector<std::pair<int, int>> arcs;
    std::ifstream gr_file(file_name);

    int nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;
    arcs.reserve(nb_arcs);

    int from, to;
    while(gr_file >> from >> to) {
        arcs.emplace_back(from, to);
    }

    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    return graph_t(boost::edges_are_sorted, arcs.begin(), arcs.end(), nb_nodes);
}
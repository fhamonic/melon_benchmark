#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include "input_file.hpp"

template <typename _Graph>
void parse_snap(const std::filesystem::path & file_name, _Graph & graph) {
    auto gr_file = open_input_file(file_name);
    int nb_nodes = 0, nb_arcs = 0;
    gr_file >> nb_nodes >> nb_arcs;

    for(std::size_t i = 0; i < nb_nodes; ++i) {
        graph.addNode();
    }

    int from, to;
    while(gr_file >> from >> to) {
        graph.addArc(graph.fromId(from, typename _Graph::Node()),
                     graph.fromId(to, typename _Graph::Node()));
    }
}

void parse_snap(const std::filesystem::path & file_name,
                lemon::StaticDigraph & graph) {
    auto gr_file = open_input_file(file_name);
    int nb_nodes = 0, nb_arcs = 0;
    gr_file >> nb_nodes >> nb_arcs;

    std::vector<std::pair<int, int>> arcs;
    arcs.reserve(nb_arcs);
    int from, to;
    while(gr_file >> from >> to) {
        arcs.push_back(std::make_pair(from, to));
    }
    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    graph.build(nb_nodes, arcs.begin(), arcs.end());
}
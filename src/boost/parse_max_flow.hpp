#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/graph/adjacency_list.hpp>

// Boost's max-flow algorithms work on an explicit residual network: every arc
// needs a reverse arc, and each of the two must point at the other through an
// edge_reverse_t property. That is why max-flow needs its own reader rather
// than reusing parse_dimacs.hpp.
template <typename _Value>
using max_flow_graph = boost::adjacency_list<
    boost::vecS, boost::vecS, boost::directedS, boost::no_property,
    boost::property<
        boost::edge_capacity_t, _Value,
        boost::property<
            boost::edge_residual_capacity_t, _Value,
            boost::property<boost::edge_reverse_t,
                            boost::adjacency_list_traits<
                                boost::vecS, boost::vecS,
                                boost::directedS>::edge_descriptor>>>>;

template <typename _Value>
[[nodiscard]] max_flow_graph<_Value> parse_max_flow_dimacs(
    const std::filesystem::path & file_name) {
    using graph_t = max_flow_graph<_Value>;

    std::size_t nb_nodes = 0;
    std::vector<std::tuple<std::size_t, std::size_t, _Value>> arcs;

    std::ifstream gr_file(file_name);
    std::string line;
    while(getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(!(iss >> ch)) continue;
        switch(ch) {
            case 'c':
            case 'n':
                break;
            case 'p': {
                std::string format;
                std::size_t nb_arcs;
                if(iss >> format >> nb_nodes >> nb_arcs) arcs.reserve(nb_arcs);
                break;
            }
            case 'a': {
                std::size_t from, to;
                _Value capacity;
                if(iss >> from >> to >> capacity)
                    arcs.emplace_back(from - 1, to - 1, capacity);
                break;
            }
            default:
                std::cerr << "Error in reading " << file_name << " unknown '"
                          << ch << '\'' << std::endl;
                std::abort();
        }
    }

    graph_t graph(nb_nodes);
    auto capacity_map = get(boost::edge_capacity, graph);
    auto reverse_map = get(boost::edge_reverse, graph);

    for(const auto & [u, v, capacity] : arcs) {
        const auto [arc, arc_inserted] = add_edge(u, v, graph);
        // Zero-capacity twin carrying the reverse residual.
        const auto [rev, rev_inserted] = add_edge(v, u, graph);
        capacity_map[arc] = capacity;
        capacity_map[rev] = _Value{0};
        reverse_map[arc] = rev;
        reverse_map[rev] = arc;
    }

    return graph;
}

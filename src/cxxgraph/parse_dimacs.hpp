#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "input_file.hpp"

#include "CXXGraph/CXXGraph.hpp"

// CXXGraph models a graph as a set of shared_ptr edges over shared_ptr nodes,
// each node carrying a std::string user id. There is no notion of a vertex
// index, so the node's payload is set to its 0-based DIMACS index and the
// digests read it back through getData() -- that is what lets a CXXGraph
// result be compared against MELON's, LEMON's and Boost's.
using cxxgraph_node = CXXGraph::Node<unsigned int>;

struct cxxgraph_instance {
    CXXGraph::Graph<unsigned int> graph;
    // Indexed by DIMACS vertex id, so sources can be looked up by index the
    // same way every other library looks them up.
    std::vector<CXXGraph::shared<const cxxgraph_node>> nodes;
};

// CXXGraph's dijkstra refuses any edge whose isWeighted() is false
// (ERR_NO_WEIGHTED_EDGE), so the shortest-path benchmark has to build
// DirectedWeightedEdge while the traversals build the cheaper DirectedEdge.
template <bool _Weighted = false>
[[nodiscard]] inline cxxgraph_instance parse_dimacs(
    const std::filesystem::path & file_name) {
    cxxgraph_instance instance;
    CXXGraph::id_t arc_id = 0;

    auto gr_file = open_input_file(file_name);
    std::string line;
    while(std::getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(!(iss >> ch)) continue;
        switch(ch) {
            case 'c':
            case 'n':
                break;
            case 'p': {
                std::string format;
                std::size_t nb_nodes, nb_arcs;
                if(iss >> format >> nb_nodes >> nb_arcs) {
                    instance.nodes.reserve(nb_nodes);
                    for(std::size_t i = 0; i < nb_nodes; ++i)
                        instance.nodes.push_back(
                            std::make_shared<const cxxgraph_node>(
                                std::to_string(i),
                                static_cast<unsigned int>(i)));
                }
                break;
            }
            case 'a': {
                std::size_t from, to;
                long length;
                if(iss >> from >> to >> length) {
                    if constexpr(_Weighted) {
                        instance.graph.addEdge(
                            std::make_shared<
                                const CXXGraph::DirectedWeightedEdge<
                                    unsigned int>>(
                                arc_id++, instance.nodes[from - 1],
                                instance.nodes[to - 1],
                                static_cast<double>(length)));
                    } else {
                        instance.graph.addEdge(
                            std::make_shared<
                                const CXXGraph::DirectedEdge<unsigned int>>(
                                arc_id++, instance.nodes[from - 1],
                                instance.nodes[to - 1]));
                    }
                }
                break;
            }
            default:
                std::cerr << "Error in reading " << file_name << " unknown '"
                          << ch << '\'' << std::endl;
                std::abort();
        }
    }
    return instance;
}

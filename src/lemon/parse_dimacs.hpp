#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "helper.hpp"
#include "input_file.hpp"

template <typename _Value>
struct arc_entry {
    int first;
    int second;
    _Value weight;
    arc_entry(int u, int v, _Value l) : first(u), second(v), weight(l) {}
};

template <typename _Graph, typename _Value>
void parse_dimacs(const std::filesystem::path & file_name, _Graph & graph,
                  typename _Graph::ArcMap<_Value> & length_map) {
    if constexpr(std::same_as<_Graph, StaticDigraph>) {
        std::vector<arc_entry<_Value>> arcs;
        std::string format;
        int nb_nodes = 0, nb_arcs = 0;

        auto gr_file = open_input_file(file_name);
        std::string line;
        while(getline(gr_file, line)) {
            std::istringstream iss(line);
            char ch;
            if(iss >> ch) {
                switch(ch) {
                    case 'c':
                        break;
                    case 'n':
                        break;
                    case 'p': {
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
                        std::cerr << "Error in reading " << file_name
                                  << " unknown '" << ch << '\'' << std::endl;
                        std::abort();
                }
            }
        }

        std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
            if(a.first == b.first) return a.second < b.second;
            return a.first < b.first;
        });
        graph.build(nb_nodes, arcs.begin(), arcs.end());
        for(std::size_t i = 0; i < nb_arcs; ++i) {
            length_map[graph.arcFromId(i)] = arcs[i].weight;
        }
    } else {
        auto gr_file = open_input_file(file_name);
        std::string line;
        while(getline(gr_file, line)) {
            std::istringstream iss(line);
            char ch;
            if(iss >> ch) {
                switch(ch) {
                    case 'c':
                        break;
                    case 'n':
                        break;
                    case 'p': {
                        std::string format;
                        int nb_nodes = 0, nb_arcs = 0;
                        if(iss >> format >> nb_nodes >> nb_arcs) {
                            for(int i = 0; i < nb_nodes; ++i) {
                                graph.addNode();
                            }
                        }
                        break;
                    }
                    case 'a': {
                        int from, to;
                        _Value length;
                        if(iss >> from >> to >> length) {
                            auto a = graph.addArc(graph.nodeFromId(from - 1),
                                                  graph.nodeFromId(to - 1));
                            length_map[a] = length;
                        }
                        break;
                    }
                    default:
                        std::cerr << "Error in reading " << file_name
                                  << " unknown '" << ch << '\'' << std::endl;
                        std::abort();
                }
            }
        }
    }
}

template <typename _Graph, typename _Value>
void parse_undirected_dimacs(const std::filesystem::path & file_name,
                             _Graph & graph,
                             typename _Graph::EdgeMap<_Value> & length_map) {
    auto gr_file = open_input_file(file_name);
    std::string line;
    while(getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(iss >> ch) {
            switch(ch) {
                case 'c':
                    break;
                case 'n':
                    break;
                case 'p': {
                    std::string format;
                    int nb_nodes = 0, nb_arcs = 0;
                    if(iss >> format >> nb_nodes >> nb_arcs) {
                        for(int i = 0; i < nb_nodes; ++i) {
                            graph.addNode();
                        }
                    }
                    break;
                }
                case 'a': {
                    int from, to;
                    _Value length;
                    if(iss >> from >> to >> length) {
                        auto a = graph.addEdge(graph.nodeFromId(from - 1),
                                               graph.nodeFromId(to - 1));
                        length_map[a] = length;
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name
                              << " unknown '" << ch << '\'' << std::endl;
                    std::abort();
            }
        }
    }
}
// A parsed instance, owned in one place so that cached_parse_into can hand it
// out by reference: LEMON's graphs are neither copyable nor movable, and its
// maps hold a pointer to the graph they were built from, so an instance can
// only be built where it will live. See helper.hpp for why it is built once
// per (type, file) rather than once per repetition.
template <typename _Graph, typename _Value>
struct dimacs_instance {
    _Graph graph;
    typename _Graph::template ArcMap<_Value> length_map{graph};
};

template <typename _Graph, typename _Value>
[[nodiscard]] dimacs_instance<_Graph, _Value> & cached_parse_dimacs(
    const std::filesystem::path & gr_file) {
    return cached_parse_into<dimacs_instance<_Graph, _Value>>(
        gr_file, [&](auto & instance) {
            parse_dimacs<_Graph, _Value>(gr_file, instance.graph,
                                         instance.length_map);
        });
}

template <typename _Graph, typename _Value>
struct undirected_dimacs_instance {
    _Graph graph;
    typename _Graph::template EdgeMap<_Value> costs{graph};
};

template <typename _Graph, typename _Value>
[[nodiscard]] undirected_dimacs_instance<_Graph, _Value> &
cached_parse_undirected_dimacs(const std::filesystem::path & gr_file) {
    return cached_parse_into<undirected_dimacs_instance<_Graph, _Value>>(
        gr_file, [&](auto & instance) {
            parse_undirected_dimacs<_Graph, _Value>(gr_file, instance.graph,
                                                    instance.costs);
        });
}

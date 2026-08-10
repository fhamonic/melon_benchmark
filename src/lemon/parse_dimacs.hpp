#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

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
        int nb_nodes, nb_arcs;

        std::ifstream gr_file(file_name);
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
        std::ifstream gr_file(file_name);
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
                        int nb_nodes, nb_arcs;
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
    std::ifstream gr_file(file_name);
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
                    int nb_nodes, nb_arcs;
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
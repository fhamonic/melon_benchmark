#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

#include "input_file.hpp"

template <typename V, typename AW>
using vector_graph = std::vector<std::vector<std::pair<V, AW>>>;

template <typename V, typename AW>
constexpr auto vertices(const vector_graph<V, AW> & g) {
    return std::views::iota(V{0}, static_cast<V>(g.size()));
}
template <typename V, typename AW>
constexpr auto out_arcs(const vector_graph<V, AW> & g, const V & v) {
    return std::views::iota(g[v].cbegin(), g[v].cend());
}
template <typename V, typename AW>
constexpr V arc_target(const vector_graph<V, AW> &, const auto & a) {
    return a->first;
}

using vector_cpo_int = vector_graph<unsigned int, int>;
using vector_cpo_double = vector_graph<unsigned int, double>;

template <typename W>
constexpr decltype(auto) create_vertex_map(const vector_cpo_int & g) {
    return std::vector<W>(g.size());
}
template <typename W>
constexpr decltype(auto) create_vertex_map(const vector_cpo_int & g,
                                           const W & d) {
    return std::vector<W>(g.size(), d);
}
template <typename W>
constexpr decltype(auto) create_vertex_map(const vector_cpo_double & g) {
    return std::vector<W>(g.size());
}
template <typename W>
constexpr decltype(auto) create_vertex_map(const vector_cpo_double & g,
                                           const W & d) {
    return std::vector<W>(g.size(), d);
}

#include "melon/graph.hpp"

#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"
#include "melon/utility/static_digraph_builder.hpp"

template <typename _Graph, typename _Value>
auto parse_dimacs(const std::filesystem::path & gr_file) {
    if constexpr(std::same_as<_Graph, melon::mutable_digraph>) {
        melon::mutable_digraph graph;
        std::vector<_Value> lengths;

        auto file = open_input_file(gr_file);
        std::string line;
        std::size_t line_no = 0;
        while(getline(file, line)) {
            ++line_no;
            std::istringstream iss(line);
            char ch;
            if(iss >> ch) {
                switch(ch) {
                    case 'c':
                        break;
                    case 'p': {
                        std::string format;
                        std::size_t nb_nodes = 0, nb_arcs = 0;
                        if(iss >> format >> nb_nodes >> nb_arcs) {
                            for(std::size_t i = 0u; i < nb_nodes; ++i) {
                                (void)graph.create_vertex();
                            }
                            lengths.resize(nb_arcs);
                        }
                        break;
                    }
                    case 'a': {
                        melon::vertex_t<melon::mutable_digraph> from, to;
                        _Value length;
                        if(iss >> from >> to >> length) {
                            auto a = graph.create_arc(from - 1, to - 1);
                            lengths[a] = length;
                        }
                        break;
                    }
                    case 'n': {
                        break;
                    }
                    default:
                        std::cerr << "Error in reading " << gr_file << ":"
                                  << line_no << std::endl;
                        std::abort();
                }
            }
        }

        return std::make_pair(graph, lengths);
    } else if constexpr(std::same_as<_Graph, vector_cpo_int> ||
                        std::same_as<_Graph, vector_cpo_double>) {
        _Graph graph;

        auto file = open_input_file(gr_file);
        std::string line;
        std::size_t line_no = 0;
        while(getline(file, line)) {
            ++line_no;
            std::istringstream iss(line);
            char ch;
            if(iss >> ch) {
                switch(ch) {
                    case 'c':
                        break;
                    case 'p': {
                        std::string format;
                        std::size_t nb_nodes = 0, nb_arcs = 0;
                        if(iss >> format >> nb_nodes >> nb_arcs) {
                            graph.resize(nb_nodes);
                        }
                        break;
                    }
                    case 'a': {
                        melon::vertex_t<_Graph> from, to;
                        _Value length;
                        if(iss >> from >> to >> length) {
                            graph[from - 1].emplace_back(to - 1, length);
                        }
                        break;
                    }
                    case 'n': {
                        break;
                    }
                    default:
                        std::cerr << "Error in reading " << gr_file << ":"
                                  << line_no << std::endl;
                        std::abort();
                }
            }
        }

        return std::make_pair(graph, [](auto & a) { return a->second; });
    } else {
        melon::static_digraph_builder<_Graph, _Value> builder(0);

        auto file = open_input_file(gr_file);
        std::string line;
        std::size_t line_no = 0;
        while(getline(file, line)) {
            ++line_no;
            std::istringstream iss(line);
            char ch;
            if(iss >> ch) {
                switch(ch) {
                    case 'c':
                        break;
                    case 'p': {
                        std::string format;
                        std::size_t nb_nodes = 0, nb_arcs = 0;
                        if(iss >> format >> nb_nodes >> nb_arcs) {
                            builder =
                                melon::static_digraph_builder<_Graph, _Value>(
                                    nb_nodes);
                        }
                        break;
                    }
                    case 'a': {
                        melon::vertex_t<_Graph> from, to;
                        _Value length;
                        if(iss >> from >> to >> length) {
                            builder.add_arc({from - 1, to - 1}, length);
                        }
                        break;
                    }
                    case 'n': {
                        break;
                    }
                    default:
                        std::cerr << "Error in reading " << gr_file << ":"
                                  << line_no << std::endl;
                        std::abort();
                }
            }
        }

        return builder.build();
    }
}
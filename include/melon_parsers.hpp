#ifndef MELON_PARSER_HPP
#define MELON_PARSER_HPP

#include <filesystem>
#include <fstream>
#include <sstream>

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include "melon/static_digraph_builder.hpp"
#include "melon/static_forward_weighted_digraph.hpp"

template <typename G, typename W>
auto parse_melon_weighted_digraph(const std::filesystem::path & file_name) {
    fhamonic::melon::static_digraph_builder<G, W> builder(0);

    std::ifstream gr_file(file_name);
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
                    std::size_t nb_nodes, nb_arcs;
                    if(iss >> format >> nb_nodes >> nb_arcs) {
                        builder = fhamonic::melon::static_digraph_builder<G, W>(
                            nb_nodes);
                    }
                    break;
                }
                case 'a': {
                    fhamonic::melon::vertex_t<G> from, to;
                    W length;
                    if(iss >> from >> to >> length) {
                        builder.add_arc(from - 1, to - 1, length);
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name << std::endl;
                    std::abort();
            }
        }
    }

    return builder.build();
}

template <typename W>
auto parse_melon_static_forward_weighted_digraph(
    const std::filesystem::path & file_name) {
    using Graph = fhamonic::melon::static_forward_weighted_digraph<W>;

    std::string format;
    std::size_t nb_nodes, nb_arcs;

    std::vector<fhamonic::melon::vertex_t<Graph>> arcs_sources;
    std::vector<std::pair<fhamonic::melon::vertex_t<Graph>, double>> arcs;

    std::ifstream gr_file(file_name);
    std::string line;
    while(getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(iss >> ch) {
            switch(ch) {
                case 'c':
                    break;
                case 'p': {
                    iss >> format >> nb_nodes >> nb_arcs;
                    break;
                }
                case 'a': {
                    fhamonic::melon::vertex_t<Graph> from, to;
                    W length;
                    if(iss >> from >> to >> length) {
                        arcs_sources.push_back(from - 1);
                        arcs.emplace_back(to - 1, length);
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name << std::endl;
                    std::abort();
            }
        }
    }

    auto zipped = ranges::view::zip(arcs_sources, arcs);
    ranges::sort(zipped, [](const auto & a, const auto & b) {
        if(std::get<0>(a) == std::get<0>(b))
            return std::get<1>(a).first < std::get<1>(b).first;
        return std::get<0>(a) < std::get<0>(b);
    });

    return Graph(nb_nodes, arcs_sources, arcs);
}

#endif  // MELON_PARSER_HPP
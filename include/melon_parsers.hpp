#ifndef MELON_PARSER_HPP
#define MELON_PARSER_HPP

#include <filesystem>
#include <fstream>
#include <sstream>

#include "melon/static_digraph_builder.hpp"

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

#endif  // MELON_PARSER_HPP
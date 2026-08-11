#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "input_file.hpp"

#include "melon/container/mutable_digraph.hpp"
#include "melon/utility/static_digraph_builder.hpp"

template <typename _Graph>
auto parse_snap(const std::filesystem::path & file_name) {
    if constexpr(std::same_as<_Graph, mutable_digraph>) {
        melon::mutable_digraph graph;

        auto gr_file = open_input_file(file_name);
        std::size_t nb_nodes = 0, nb_arcs = 0;
        gr_file >> nb_nodes >> nb_arcs;

        for(std::size_t i = 0u; i < nb_nodes; ++i) {
            (void)graph.create_vertex();
        }

        melon::vertex_t<melon::static_digraph> from, to;
        while(gr_file >> from >> to) (void)graph.create_arc(from, to);

        return graph;
    } else {
        auto gr_file = open_input_file(file_name);
        std::size_t nb_nodes = 0, nb_arcs = 0;
        gr_file >> nb_nodes >> nb_arcs;

        melon::static_digraph_builder<melon::static_digraph> builder(nb_nodes);

        melon::vertex_t<melon::static_digraph> from, to;
        while(gr_file >> from >> to) builder.add_arc(from, to);

        auto [graph] = builder.build();
        return graph;
    }
}
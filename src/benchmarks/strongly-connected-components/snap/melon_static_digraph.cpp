#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "chrono.hpp"

#include "melon/algorithm/strongly_connected_components.hpp"
#include "melon/container/static_digraph.hpp"
#include "melon/utility/static_digraph_builder.hpp"

#include "warm_up.hpp"

using namespace fhamonic::melon;

auto parse_gr(const std::filesystem::path & file_name) {
    std::ifstream gr_file(file_name);
    std::size_t nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    static_digraph_builder<static_digraph> builder(nb_nodes);

    vertex_t<static_digraph> from, to;
    while(gr_file >> from >> to) builder.add_arc(from, to);

    return builder.build();
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,nb_components,count,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        auto [graph] = parse_gr(gr_file);
        const int nb_nodes = graph.nb_vertices();

        Chrono chrono;

        int count = 0;
        int nb_components = 0;
        for(auto && component : strongly_connected_components(graph)) {
            ++nb_components;
            // for(auto && v : component) {
            //     ++count;
            // }
        }

        double time_ms = (chrono.timeUs() / 1000.0);

        std::cout << gr_file.stem() << ',' << nb_nodes << ',' << graph.nb_arcs()
                  << ',' << nb_components << ',' << count << ',' << time_ms
                  << std::endl;
    }
    return 0;
}
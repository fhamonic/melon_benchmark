#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "chrono.hpp"

#include "melon/all.hpp"
#include "warm_up.hpp"

using namespace fhamonic::melon;

auto parse_gr(const std::filesystem::path & file_name) {
    std::ifstream gr_file(file_name);
    std::size_t nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    static_digraph_builder<> builder(nb_nodes);

    static_digraph::vertex from, to;
    while(gr_file >> from >> to) builder.add_arc(from, to);

    return builder.build();
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        auto [graph] = parse_gr(gr_file);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = graph.nb_vertices();
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        for(auto && s : graph.vertices()) {
            Chrono chrono;

            int sum = 0;
            Dfs<static_digraph> dfs(graph);
            dfs.add_source(s);

            while(!dfs.empty_queue()) {
                const auto & [a ,u] = dfs.next_node();
                sum += u;
            }
            // for(const auto & [a ,u] : dfs) {
            //     sum += u;
            // }

            double time_ms = (chrono.timeUs() / 1000.0);
            avg_time += time_ms;
            ++iterations;
            if(iterations >= nb_iterations) break;
        }
        avg_time /= iterations;

        std::cout << gr_file.stem() << ',' << nb_nodes << ',' << graph.nb_arcs()
                  << ',' << avg_time << std::endl;
    }
    return 0;
}
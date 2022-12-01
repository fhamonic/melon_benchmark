#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include "chrono.hpp"

#include "melon/algorithm/dijkstra.hpp"
#include "melon/static_forward_weighted_digraph.hpp"

#include "warm_up.hpp"

using namespace fhamonic::melon;

auto parse_gr(const std::filesystem::path & file_name) {
    using Graph = fhamonic::melon::static_forward_weighted_digraph<int>;
    std::ifstream gr_file(file_name);
    std::size_t nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    std::vector<fhamonic::melon::vertex_t<Graph>> arcs_sources;
    std::vector<std::pair<fhamonic::melon::vertex_t<Graph>, int>> arcs;

    vertex_t<Graph> from, to;
    while(gr_file >> from >> to) {
        arcs_sources.push_back(from);
        arcs.emplace_back(to, 1);
    }

    auto zipped = ranges::view::zip(arcs_sources, arcs);
    ranges::sort(zipped, [](const auto & a, const auto & b) {
        if(std::get<0>(a) == std::get<0>(b))
            return std::get<1>(a).first < std::get<1>(b).first;
        return std::get<0>(a) < std::get<0>(b);
    });

    return Graph(nb_nodes, arcs_sources, arcs);
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        auto graph = parse_gr(gr_file);

        auto length_map = graph.weights_map();

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = graph.nb_vertices();
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        for(auto && s : graph.vertices()) {
            Chrono chrono;

            int sum = 0;
            dijkstra algo(graph, length_map);
            algo.add_source(s);
            while(!algo.finished()) {
                auto [u, dist] = algo.current();
                algo.advance();
                sum += dist;
            }

            for(auto && [u, dist] : algo) {
                sum += dist;
            }

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
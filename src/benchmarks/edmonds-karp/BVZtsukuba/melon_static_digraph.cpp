#include <filesystem>
#include <iostream>
#include <type_traits>

#include "melon/algorithm/edmonds_karp.hpp"
#include "melon/container/static_digraph.hpp"

#include "chrono.hpp"
#include "melon_parsers.hpp"
#include "warm_up.hpp"

using namespace fhamonic::melon;

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/BVZ-tsukuba/BVZ-tsukuba0.max",
         "data/BVZ-tsukuba/BVZ-tsukuba1.max",
         "data/BVZ-tsukuba/BVZ-tsukuba2.max",
         "data/BVZ-tsukuba/BVZ-tsukuba3.max",
         "data/BVZ-tsukuba/BVZ-tsukuba4.max",
         "data/BVZ-tsukuba/BVZ-tsukuba5.max",
         "data/BVZ-tsukuba/BVZ-tsukuba6.max",
         "data/BVZ-tsukuba/BVZ-tsukuba7.max",
         "data/BVZ-tsukuba/BVZ-tsukuba8.max",
         "data/BVZ-tsukuba/BVZ-tsukuba9.max",
         "data/BVZ-tsukuba/BVZ-tsukuba10.max",
         "data/BVZ-tsukuba/BVZ-tsukuba11.max",
         "data/BVZ-tsukuba/BVZ-tsukuba12.max",
        //  "data/BVZ-tsukuba/BVZ-tsukuba13.max",
         "data/BVZ-tsukuba/BVZ-tsukuba14.max",
         "data/BVZ-tsukuba/BVZ-tsukuba15.max"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms,optimum\n";

    (void)warm_up();

    int sum = 0;
    for(const auto & gr_file : gr_files) {
        auto [graph, capacity_map] =
            parse_melon_weighted_digraph<static_digraph, int>(gr_file);

        const int nb_nodes = graph.nb_vertices();
        Chrono chrono;

        edmonds_karp algo(graph, capacity_map, 0u, 1u);
        algo.run();

        sum += algo.flow_value();

        int time_ms = (chrono.timeUs() / 1000.0);

        std::cout << gr_file.stem() << ',' << nb_nodes << ',' << graph.nb_arcs()
                  << ',' << time_ms << ',' << algo.flow_value() << std::endl;
    }
    return 0;
}
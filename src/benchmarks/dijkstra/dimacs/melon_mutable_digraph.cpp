#include <filesystem>
#include <iostream>
#include <type_traits>

#include "melon/algorithm/dijkstra.hpp"
#include "melon/container/static_digraph.hpp"
#include "melon/container/mutable_digraph.hpp"

#include "chrono.hpp"
#include "melon_parsers.hpp"
#include "warm_up.hpp"

using namespace fhamonic::melon;

struct dijkstra_traits {
    using semiring = shortest_path_semiring<double>;
    using heap = d_ary_heap<2, vertex_t<static_digraph>, double,
                            decltype([](const auto & e1, const auto & e2) {
                                return semiring::less(e1.second, e2.second);
                            }),
                            vertex_map_t<static_digraph, std::size_t>>;

    static constexpr bool store_paths = false;
    static constexpr bool store_distances = false;
};

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.FLA.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.FLA.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.NW.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.NW.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.NE.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.NE.gr"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        auto [sgraph, slength_map] =
            parse_melon_weighted_digraph<static_digraph, double>(gr_file);

        mutable_digraph graph;
        for(auto && v : vertices(sgraph)) {
            graph.create_vertex();
        }
        for(auto && [a, arc_pair] : arcs_entries(sgraph)) {
            graph.create_arc(arc_pair.first, arc_pair.second);
        }
        auto length_map = create_arc_map<double>(graph);
        std::size_t cpt = 0;
        for(auto && [a, arc_pair] : arcs_entries(sgraph)) {
            length_map[cpt] = slength_map[a];
            ++cpt;
        }

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = graph.nb_vertices();
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        for(auto && s : graph.vertices()) {
            Chrono chrono;

            double sum = 0;
            dijkstra<decltype(graph), decltype(length_map), dijkstra_traits>
                algo(graph, length_map);
            algo.add_source(s);

            // while(!algo.finished()) {
            //     const auto & [u, dist] = algo.current();
            //     sum += dist;
            //     algo.advance();
            // }

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
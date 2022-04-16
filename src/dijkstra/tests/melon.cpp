#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "melon/all.hpp"

using namespace fhamonic::melon;

auto parse_gr(std::string file_name) {
    static_digraph_builder<double> builder(0);

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
                        builder = static_digraph_builder<double>(nb_nodes);
                    }
                    break;
                }
                case 'a': {
                    static_digraph::vertex from, to;
                    double length;
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

int main() {
    std::vector<std::string> gr_files({
        // "data/rome99.gr"
        //,
        "data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr"
        //,
        //  "data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr",
        //  "data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr",
        //  "data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr",
        //  "data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr",
        //  "data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr"
    });

    int rows = 0;
    for(const auto & gr_file : gr_files) {
        auto [graph, length_map] = parse_gr(gr_file);

        std::cout << std::setprecision(16) << gr_file << " : "
                  << graph.nb_vertices() << " nodes , " << graph.nb_arcs()
                  << " arcs" << std::endl;

        for(auto && s : graph.vertices()) {
            // Dijkstra<StaticDigraph, std::vector<double>, TraversalAlgorithmBehavior::TRACK_NONE> dijkstra(graph, length_map);
            // Dijkstra dijkstra(graph, length_map);
            // dijkstra.add_source(s);
            // std::vector<double> dists(graph.nb_vertices());
            // for(const auto & [u, dist] : dijkstra) {
            //     dists[u] = dist;
            // }
            // for(StaticDigraph::Node u : graph.vertices()) {
            //     std::cout << s << ',' << u << ':' << dists[u] << '\n';
            //     ++rows;
            // }

            Dijkstra<static_digraph, std::vector<double>, TraversalAlgorithmBehavior::TRACK_DISTANCES> dijkstra(graph, length_map);
            dijkstra.add_source(s);
            dijkstra.run();
            for(auto && u : graph.vertices()) {
                std::cout << s << ',' << u << ':' << dijkstra.dist(u) << '\n';
                ++rows;
            }
            if(rows > 1000000) goto finish;
        }
    }
finish:
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// #include "melon_rend-indexed.hpp"
// #include "melon_1-index.hpp"
#include "melon.hpp"

using namespace fhamonic::melon;

auto parse_gr(std::string file_name) {
    StaticDigraphBuilder<double> builder(0);

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
                        builder = StaticDigraphBuilder<double>(nb_nodes);
                    }
                    break;
                }
                case 'a': {
                    StaticDigraph::Node from, to;
                    double length;
                    if(iss >> from >> to >> length) {
                        builder.addArc(from - 1, to - 1, length);
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

        std::cout << std::setprecision(16) << gr_file << " : " << graph.nb_nodes() << " nodes , "
                  << graph.nb_arcs() << " arcs" << std::endl;

        for(StaticDigraph::Node s : graph.nodes()) {
            Dijkstra dijkstra(graph, length_map);
            dijkstra.reset();
            dijkstra.addSource(s);
            std::vector<double> dists(graph.nb_nodes());
            while(!dijkstra.emptyQueue()) {
                auto [u, dist] = dijkstra.processNextNode();
                dists[u] = dist;
            }

            for(StaticDigraph::Node u : graph.nodes()) {
                std::cout << s << ',' << u << ':' << dists[u] << '\n';
                ++rows;
            }
            if(rows > 20000000) goto finish;
        }
    }
finish:
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
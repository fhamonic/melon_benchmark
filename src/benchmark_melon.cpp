#include <fstream>
#include <iostream>
#include <sstream>

#include "chrono.hpp"

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
    std::vector<std::string> gr_files(
        {"data/rome99.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr",
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

    for(const auto & gr_file : gr_files) {
        auto [graph, length_map] = parse_gr(gr_file);

        const int nb_nodes = graph.nb_nodes();
        std::cout << gr_file << " : " << nb_nodes << " nodes , "
                  << graph.nb_arcs() << " arcs" << std::endl;

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_iterations = 30000.0 * 10000.0 / nb_nodes;
        for(StaticDigraph::Node s : graph.nodes()) {
            Chrono chrono;

            double sum = 0;
            Dijkstra dijkstra(graph, length_map);
            dijkstra.reset();
            dijkstra.addSource(s);
            while(!dijkstra.emptyQueue()) {
                auto [u, dist] = dijkstra.processNextNode();
                sum += dist;
            }

            double time_ms = (chrono.timeUs() / 1000.0);
            // std::cout << "Dijkstra from " << s << " takes " << time_ms
            //           << " ms, sum dists = " << sum << std::endl;

            avg_time += time_ms;
            ++iterations;
            if(iterations >= nb_iterations) break;
        }
        avg_time /= iterations;

        std::cout << "avg time Dijkstra : " << avg_time << " ms" << std::endl;
    }

    return EXIT_SUCCESS;
}
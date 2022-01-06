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
                        builder.addArc(from, to, length);
                    }
                    break;
                }
                default:
                    assert(false);
            }
        }
    }

    return builder.build();
}

int main() {
    std::vector<std::string> gr_files(
        {"data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr"});

    for(const auto gr_file : gr_files) {
        auto [graph, lenght_map] = parse_gr(gr_file);

        std::cout << gr_file << " : " << graph.nb_nodes() << " nodes , " << graph.nb_arcs() << " arcs" << std::endl;

        for(StaticDigraph::Node u : graph.nodes()) {
            Chrono chrono;

            Dijkstra dijkstra(graph, lenght_map);
            dijkstra.init(u);
            while(!dijkstra.emptyQueue()) {
                (void) dijkstra.processNextNode();
            }

            std::cout << "Dijkstra from " << u << " takes " << chrono.timeMs() << " ms" << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
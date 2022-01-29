#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "chrono.hpp"
#include "warm_up.hpp"

typedef adjacency_list<vecS, vecS, directedS, no_property,
                       property<edge_weight_t, double>>
    graph_t;
typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;
typedef std::pair<int, int> Edge;

void parse_gr(const std::filesystem::path & file_name, graph_t & graph,
              property_map<graph_t, edge_weight_t>::type & length_map) {
    int nb_nodes;
    std::vector<std::tuple<int, int, double>> arcs;

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
                    int nb_arcs;
                    iss >> format >> nb_nodes >> nb_arcs;
                    break;
                }
                case 'a': {
                    int from, to;
                    double length;
                    if(iss >> from >> to >> length) {
                        arcs.emplace_back(from - 1, to - 1, length);
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name << std::endl;
                    std::abort();
            }
        }
    }

    graph = graph_t(nb_nodes);
    length_map = get(edge_weight, graph);
    for(const auto & [u, v, length] : arcs) {
        edge_descriptor e;
        bool inserted;
        tie(e, inserted) = add_edge(u, v, graph);
        length_map[e] = length;
    }
}

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
        graph_t graph;
        property_map<graph_t, edge_weight_t>::type length_map;
        parse_gr(gr_file, graph, length_map);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = num_vertices(graph);
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        graph_traits<graph_t>::vertex_iterator si, send;
        for(tie(si, send) = vertices(graph); si != send; ++si) {
            std::vector<vertex_descriptor> p(nb_nodes);
            std::vector<int> d(nb_nodes);
            vertex_descriptor s = *si;

            Chrono chrono;
            double sum = 0;
            dijkstra_shortest_paths(graph, s,
                                    predecessor_map(&p[0]).distance_map(&d[0]));

            graph_traits<graph_t>::vertex_iterator vi, vend;
            for(tie(vi, vend) = vertices(graph); vi != vend; ++vi) {
                sum += d[*vi];
            }

            double time_ms = (chrono.timeUs() / 1000.0);
            avg_time += time_ms;
            ++iterations;
            if(iterations >= nb_iterations) break;
        }
        avg_time /= iterations;

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << num_edges(graph) << ',' << avg_time << std::endl;
    }
    return 0;
}
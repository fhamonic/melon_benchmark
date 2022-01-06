#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/dijkstra.h>

#include "chrono.hpp"

using namespace lemon;

void parse_gr(std::string file_name, ListDigraph & graph,
              ListDigraph::ArcMap<double> & length_map) {
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
                    int nb_nodes, nb_arcs;
                    if(iss >> format >> nb_nodes >> nb_arcs) {
                        for(int i = 0; i < nb_nodes; ++i) {
                            graph.addNode();
                        }
                    }
                    break;
                }
                case 'a': {
                    int from, to;
                    double length;
                    if(iss >> from >> to >> length) {
                        auto a = graph.addArc(graph.nodeFromId(from - 1),
                                              graph.nodeFromId(to - 1));
                        length_map[a] = length;
                    }
                    break;
                }
                default:
                    std::cerr << "Error in reading " << file_name << std::endl;
                    std::abort();
            }
        }
    }
}

int main() {
    std::vector<std::string> gr_files(
        {"data/rome99.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr"});

    for(const auto gr_file : gr_files) {
        // using Graph = ListDigraph;
        // ListDigraph graph;
        // ListDigraph::ArcMap<double> length_map(graph);
        // parse_gr(gr_file, graph, length_map);

        using Graph = StaticDigraph;
        ListDigraph list_graph;
        ListDigraph::ArcMap<double> list_length_map(list_graph);
        parse_gr(gr_file, list_graph, list_length_map);
        StaticDigraph graph;
        StaticDigraph::ArcMap<double> length_map(graph);
        ListDigraph::NodeMap<StaticDigraph::Node> node_ref_map(list_graph);
        ListDigraph::ArcMap<StaticDigraph::Arc> arc_ref_map(list_graph);
        graph.build(list_graph, node_ref_map, arc_ref_map);
        for(ListDigraph::ArcIt a(list_graph); a != INVALID; ++a)
            length_map[arc_ref_map[a]] = list_length_map[a];

        std::cout << gr_file << " : " << countNodes(graph) << " nodes , "
                  << countArcs(graph) << " arcs" << std::endl;

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int n = countNodes(graph);
        for(int i = 0; i < n; ++i) {
            Graph::Node s = graph.nodeFromId(i);
            Chrono chrono;

            double sum = 0;
            Dijkstra<Graph, Graph::ArcMap<double>> dijkstra(graph, length_map);
            dijkstra.init();
            dijkstra.addSource(s);
            while(!dijkstra.emptyQueue()) {
                auto u = dijkstra.processNextNode();
                sum += dijkstra.dist(u);
            }

            double time_ms = (chrono.timeUs() / 1000.0);
            // std::cout << "Dijkstra from " << graph.id(s) << " takes " <<
            // time_ms
            //           << " ms, sum dists = " << sum << std::endl;

            avg_time += time_ms;
            ++iterations;
            if(gr_chrono.timeS() >= 30)
                break;
        }
        avg_time /= iterations;

        std::cout << "avg time Dijkstra : " << avg_time << " ms" << std::endl;
    }

    return 0;
}
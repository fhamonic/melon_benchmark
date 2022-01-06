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
                    std::size_t nb_nodes, nb_arcs;
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
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr"});

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

        for(Graph::NodeIt u(graph); u != INVALID; ++u) {
            Chrono chrono;

            Dijkstra<Graph, Graph::ArcMap<double>> dijkstra(
                graph, length_map);
            dijkstra.init();
            dijkstra.addSource(u);
            while(!dijkstra.emptyQueue()) {
                (void)dijkstra.processNextNode();
            }

            std::cout << "Dijkstra from " << graph.id(u) << " takes "
                      << (chrono.timeUs() / 1000.0) << " ms" << std::endl;
        }
    }

    return 0;
}
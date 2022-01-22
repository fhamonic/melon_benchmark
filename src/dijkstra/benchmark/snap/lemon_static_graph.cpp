#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/dijkstra.h>

#include "chrono.hpp"
#include "warm_up.hpp"

using namespace lemon;

void parse_txt(const std::filesystem::path & file_name, ListDigraph & graph,
              ListDigraph::ArcMap<int> & length_map) {
    std::ifstream gr_file(file_name);

    int nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    for(int i = 0; i < nb_nodes; ++i) graph.addNode();

    int from, to;
    while(gr_file >> from >> to) {
        ListDigraph::Arc a =
            graph.addArc(graph.nodeFromId(from), graph.nodeFromId(to));
        length_map[a] = 1;
    }
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        // using Graph = ListDigraph;
        // ListDigraph graph;
        // ListDigraph::ArcMap<double> length_map(graph);
        // parse_gr(gr_file, graph, length_map);

        using Graph = StaticDigraph;
        ListDigraph list_graph;
        ListDigraph::ArcMap<int> list_length_map(list_graph);
        parse_txt(gr_file, list_graph, list_length_map);
        StaticDigraph graph;
        StaticDigraph::ArcMap<int> length_map(graph);
        ListDigraph::NodeMap<StaticDigraph::Node> node_ref_map(list_graph);
        ListDigraph::ArcMap<StaticDigraph::Arc> arc_ref_map(list_graph);
        graph.build(list_graph, node_ref_map, arc_ref_map);
        for(ListDigraph::ArcIt a(list_graph); a != INVALID; ++a)
            length_map[arc_ref_map[a]] = list_length_map[a];

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = countNodes(graph);
        const int nb_iterations = 30000.0 * 10000.0 / nb_nodes;
        for(Graph::NodeIt s(graph); s != INVALID; ++s) {
            Chrono chrono;

            int sum = 0;
            Dijkstra<Graph, Graph::ArcMap<int>> dijkstra(graph, length_map);
            dijkstra.init();
            dijkstra.addSource(s);
            while(!dijkstra.emptyQueue()) {
                auto u = dijkstra.processNextNode();
                sum += dijkstra.dist(u);
            }

            double time_ms = (chrono.timeUs() / 1000.0);
            avg_time += time_ms;
            ++iterations;
            if(iterations >= nb_iterations) break;
        }
        avg_time /= iterations;

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << countArcs(graph) << ',' << avg_time << std::endl;
    }
    return 0;
}
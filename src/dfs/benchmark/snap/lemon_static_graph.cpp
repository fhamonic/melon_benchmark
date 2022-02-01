#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/dfs.h>

#include "chrono.hpp"
#include "warm_up.hpp"

using namespace lemon;

void parse_txt(const std::filesystem::path & file_name, ListDigraph & graph) {
    std::ifstream gr_file(file_name);

    int nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    for(int i = 0; i < nb_nodes; ++i) graph.addNode();

    int from, to;
    while(gr_file >> from >> to) {
        graph.addArc(graph.nodeFromId(from), graph.nodeFromId(to));
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
        parse_txt(gr_file, list_graph);
        StaticDigraph graph;
        ListDigraph::NodeMap<StaticDigraph::Node> node_ref_map(list_graph);
        ListDigraph::ArcMap<StaticDigraph::Arc> arc_ref_map(list_graph);
        graph.build(list_graph, node_ref_map, arc_ref_map);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = countNodes(graph);
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        for(Graph::NodeIt s(graph); s != INVALID; ++s) {
            Chrono chrono;

            int sum = 0;

            Dfs<Graph> dfs(graph);
            dfs.init();
            dfs.addSource(s);
            while(!dfs.emptyQueue()) {
                auto e = dfs.processNextArc();
                sum += graph.id(graph.target(e));
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
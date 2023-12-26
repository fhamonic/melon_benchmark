#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/list_graph.h>

#include <lemon/kruskal.h>

#include "chrono.hpp"
#include "warm_up.hpp"

using namespace lemon;

struct arc_entry {
    int first;
    int second;
    int weight;
    arc_entry(int u, int v, int l) : first(u), second(v), weight(l) {}
};
void parse_gr(const std::filesystem::path & file_name, ListGraph & graph,
              ListGraph::EdgeMap<int> & cost_map) {
    std::string format;
    int nb_nodes, nb_arcs;

    std::ifstream gr_file(file_name);
    std::string line;
    while(getline(gr_file, line)) {
        std::istringstream iss(line);
        char ch;
        if(iss >> ch) {
            switch(ch) {
                case 'c':
                    break;
                case 'n':
                    break;
                case 'p': {
                    iss >> format >> nb_nodes >> nb_arcs;
                    for(int i = 0; i < nb_nodes; ++i) graph.addNode();
                    break;
                }
                case 'a': {
                    int from, to;
                    int cost;
                    if(iss >> from >> to >> cost) {
                        auto e = graph.addEdge(graph.nodeFromId(from - 1),
                                               graph.nodeFromId(to - 1));
                        cost_map[e] = cost;
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
    std::vector<std::filesystem::path> gr_files(
        {"data/BVZ-tsukuba/BVZ-tsukuba0.max",
         "data/BVZ-tsukuba/BVZ-tsukuba1.max",
         "data/BVZ-tsukuba/BVZ-tsukuba2.max",
         "data/BVZ-tsukuba/BVZ-tsukuba3.max",
         "data/BVZ-tsukuba/BVZ-tsukuba4.max",
         "data/BVZ-tsukuba/BVZ-tsukuba5.max",
         "data/BVZ-tsukuba/BVZ-tsukuba6.max",
         "data/BVZ-tsukuba/BVZ-tsukuba7.max",
         "data/BVZ-tsukuba/BVZ-tsukuba8.max",
         "data/BVZ-tsukuba/BVZ-tsukuba9.max",
         "data/BVZ-tsukuba/BVZ-tsukuba10.max",
         "data/BVZ-tsukuba/BVZ-tsukuba11.max",
         "data/BVZ-tsukuba/BVZ-tsukuba12.max",
         //  "data/BVZ-tsukuba/BVZ-tsukuba13.max",
         "data/BVZ-tsukuba/BVZ-tsukuba14.max",
         "data/BVZ-tsukuba/BVZ-tsukuba15.max"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms,cost\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        ListGraph graph;
        ListGraph::EdgeMap<int> cost_map(graph);

        parse_gr(gr_file, graph, cost_map);

        const int nb_nodes = countNodes(graph);
        Chrono chrono;

        ListGraph::EdgeMap<bool> tree_map(graph);
        auto algo = kruskal(graph, cost_map, tree_map);

        int cost = 0;
        for(ListGraph::EdgeIt e(graph); e != INVALID; ++e) {
            if(!tree_map[e]) continue;
            cost += cost_map[e];
        }

        int time_ms = (chrono.timeUs() / 100.0);

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << countArcs(graph) << ',' << time_ms / 10.0 << ',' << cost
                  << std::endl;
    }
    return 0;
}
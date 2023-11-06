#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/static_graph.h>

#include <lemon/connectivity.h>

#include "chrono.hpp"
#include "warm_up.hpp"

using namespace lemon;

void parse_txt(const std::filesystem::path & file_name, StaticDigraph & graph) {
    std::ifstream gr_file(file_name);

    int nb_nodes, nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    std::vector<std::pair<int, int>> arcs;
    arcs.reserve(nb_arcs);
    int from, to;
    while(gr_file >> from >> to) {
        arcs.push_back(std::make_pair(from, to));
    }
    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    graph.build(nb_nodes, arcs.begin(), arcs.end());
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,nb_components,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        using Graph = StaticDigraph;
        StaticDigraph graph;
        parse_txt(gr_file, graph);
        const int nb_nodes = countNodes(graph);

        Chrono chrono;

        Graph::NodeMap<int> compMap(graph);
        int nb_components = lemon::stronglyConnectedComponents(graph, compMap);
        
        // int nb_components = lemon::countStronglyConnectedComponents(graph);

        double time_ms = (chrono.timeUs() / 1000.0);

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << countArcs(graph) << ',' << nb_components << ',' << time_ms << std::endl;
    }
    return 0;
}
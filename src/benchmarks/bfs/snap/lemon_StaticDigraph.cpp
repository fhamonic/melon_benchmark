#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/bfs.h>

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

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        using Graph = StaticDigraph;
        StaticDigraph graph;
        parse_txt(gr_file, graph);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = countNodes(graph);
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        for(int i=0; ; ++i) {
            Graph::Node s = graph.nodeFromId(i);
            Chrono chrono;

            int sum = 0;

            Bfs<Graph> bfs(graph);
            bfs.init();
            bfs.addSource(s);
            while(!bfs.emptyQueue()) {
                auto u = bfs.processNextNode();
                sum += graph.id(u);
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
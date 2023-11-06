#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/static_graph.h>

#include <lemon/edmonds_karp.h>

#include "chrono.hpp"
#include "warm_up.hpp"

using namespace lemon;

struct arc_entry {
    int first;
    int second;
    int weight;
    arc_entry(int u, int v, int l) : first(u), second(v), weight(l) {}
};
std::unique_ptr<StaticDigraph::ArcMap<int>> parse_gr(
    const std::filesystem::path & file_name, StaticDigraph & graph) {
    std::vector<arc_entry> arcs;
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
                    break;
                }
                case 'a': {
                    int from, to;
                    int length;
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

    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    graph.build(nb_nodes, arcs.begin(), arcs.end());
    auto capacity_map = std::make_unique<StaticDigraph::ArcMap<int>>(graph);
    for(std::size_t i = 0; i < nb_arcs; ++i) {
        (*capacity_map)[graph.arcFromId(i)] = arcs[i].weight;
    }
    return std::move(capacity_map);
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

    std::cout << "instance,nb_nodes,nb_arcs,time_ms,optimum\n";

    (void)warm_up();

    int sum = 0;
    for(const auto & gr_file : gr_files) {
        using Graph = StaticDigraph;
        StaticDigraph graph;
        std::unique_ptr<StaticDigraph::ArcMap<int>> capacity_map =
            parse_gr(gr_file, graph);

        const int nb_nodes = countNodes(graph);
        Chrono chrono;

        EdmondsKarp<Graph, Graph::ArcMap<int>> edmonds_karp(
            graph, *capacity_map, graph.nodeFromId(0), graph.nodeFromId(1));

        edmonds_karp.run();
        sum += edmonds_karp.flowValue();

        int time_ms = (chrono.timeUs() / 1000.0);

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << countArcs(graph) << ',' << time_ms << ','
                  << edmonds_karp.flowValue() << std::endl;
    }
    return 0;
}
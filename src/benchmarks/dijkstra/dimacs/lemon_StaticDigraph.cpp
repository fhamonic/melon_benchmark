#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <lemon/dfs.h>
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/bfs.h>
#include <lemon/dheap.h>
#include <lemon/dijkstra.h>
#include <lemon/quad_heap.h>

#include "chrono.hpp"
#include "warm_up.hpp"

using namespace lemon;

template <typename GR, typename LEN>
struct DijkstraTraits {
    typedef GR Digraph;
    typedef LEN LengthMap;
    typedef typename LEN::Value Value;
    typedef DijkstraDefaultOperationTraits<Value> OperationTraits;
    typedef typename Digraph::template NodeMap<int> HeapCrossRef;
    static HeapCrossRef * createHeapCrossRef(const Digraph & g) {
        return new HeapCrossRef(g);
    }

    // typedef BinHeap<typename LEN::Value, HeapCrossRef, std::less<Value>>
    // Heap; typedef QuadHeap<typename LEN::Value, HeapCrossRef,
    // std::less<Value>> Heap;
    typedef DHeap<typename LEN::Value, HeapCrossRef, 2, std::less<Value>> Heap;
    static Heap * createHeap(HeapCrossRef & r) { return new Heap(r); }

    typedef typename Digraph::template NodeMap<typename Digraph::Arc> PredMap;
    static PredMap * createPredMap(const Digraph & g) { return new PredMap(g); }

    typedef NullMap<typename Digraph::Node, bool> ProcessedMap;
    static ProcessedMap * createProcessedMap(const Digraph &) {
        return new ProcessedMap();
    }

    typedef typename Digraph::template NodeMap<typename LEN::Value> DistMap;
    static DistMap * createDistMap(const Digraph & g) { return new DistMap(g); }
};

struct arc_entry {
    int first;
    int second;
    double weight;
    arc_entry(int u, int v, double l) : first(u), second(v), weight(l) {}
};
std::unique_ptr<StaticDigraph::ArcMap<double>> parse_gr(
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
                case 'p': {
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

    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    graph.build(nb_nodes, arcs.begin(), arcs.end());
    auto length_map = std::make_unique<StaticDigraph::ArcMap<double>>(graph);
    for(std::size_t i = 0; i < nb_arcs; ++i) {
        (*length_map)[graph.arcFromId(i)] = arcs[i].weight;
    }
    return std::move(length_map);
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
        using Graph = StaticDigraph;
        StaticDigraph graph;
        std::unique_ptr<StaticDigraph::ArcMap<double>> length_map =
            parse_gr(gr_file, graph);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_nodes = countNodes(graph);
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        for(int i = 0;; ++i) {
            Graph::Node s = graph.nodeFromId(i);
            Chrono chrono;

            double sum = 0;
            Dijkstra<Graph, Graph::ArcMap<double>,
                     DijkstraTraits<Graph, Graph::ArcMap<double>>>
                dijkstra(graph, *length_map);

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
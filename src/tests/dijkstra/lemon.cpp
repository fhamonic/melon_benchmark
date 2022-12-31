#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/dijkstra.h>

using namespace lemon;

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
    std::vector<std::string> gr_files({
        // "data/rome99.gr"
        //,
        "data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr"
        //,
        //  "data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr",
        //  "data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr",
        //  "data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr",
        //  "data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr",
        //  "data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr"
    });

    int rows = 0;
    for(const auto & gr_file : gr_files) {
        using Graph = StaticDigraph;
        StaticDigraph graph;
        std::unique_ptr<StaticDigraph::ArcMap<double>> length_map =
            parse_gr(gr_file, graph);

        const int nb_nodes = countNodes(graph);
        std::cout << std::setprecision(16) << gr_file << " : " << nb_nodes
                  << " nodes , " << countArcs(graph) << " arcs" << std::endl;

        for(int i = 0; i < nb_nodes; ++i) {
            Graph::Node s = graph.nodeFromId(i);
            Dijkstra<Graph, Graph::ArcMap<double>> dijkstra(graph, *length_map);
            dijkstra.run(s);

            for(int j = 0; j < nb_nodes; ++j) {
                Graph::Node u = graph.nodeFromId(j);
                std::cout << graph.id(s) << ','
                          << graph.id(u) << ':'
                          << dijkstra.dist(u) << '\n';
                ++rows;
            }
            if(rows > 1000000) goto finish;
        }
    }
finish:
    std::cout << std::endl;
    return 0;
}
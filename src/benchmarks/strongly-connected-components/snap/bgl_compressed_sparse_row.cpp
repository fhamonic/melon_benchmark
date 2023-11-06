#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/strong_components.hpp>

using namespace boost;

#include "chrono.hpp"
#include "warm_up.hpp"

typedef compressed_sparse_row_graph<directedS, no_property, no_property>
    graph_t;
typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;
typedef std::pair<int, int> Edge;

void parse_gr(const std::filesystem::path & file_name, graph_t & graph) {
    std::vector<std::pair<int, int>> arcs;

    std::ifstream gr_file(file_name);

    int nb_nodes;
    int nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;

    int from, to;
    while(gr_file >> from >> to) {
        arcs.emplace_back(from, to);
    }

    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    graph = graph_t(edges_are_sorted, arcs.begin(), arcs.end(), nb_nodes);
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,nb_components,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        graph_t graph;
        parse_gr(gr_file, graph);
        const int nb_nodes = num_vertices(graph);

        Chrono chrono;

        std::vector<int> compMap(nb_nodes);
        int nb_components = boost::strong_components(
            graph, make_iterator_property_map(compMap.begin(), boost::get(boost::vertex_index, graph)));

        double time_ms = (chrono.timeUs() / 1000.0);

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << num_edges(graph) << ',' << nb_components << ',' << time_ms
                  << std::endl;
    }
    return 0;
}
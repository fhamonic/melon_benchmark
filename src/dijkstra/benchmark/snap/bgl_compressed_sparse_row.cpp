#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "chrono.hpp"
#include "warm_up.hpp"

struct Edge_Cost {
    int weight;
    Edge_Cost() {}
    Edge_Cost(int w) : weight(w) {}
};
typedef compressed_sparse_row_graph<directedS, no_property, Edge_Cost> graph_t;
typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;
typedef std::pair<int, int> Edge;

void parse_gr(const std::filesystem::path & file_name, graph_t & graph,
              std::vector<Edge_Cost> & weights) {
    std::vector<std::pair<int, int>> arcs;

    std::ifstream gr_file(file_name);

    int nb_nodes;
    int nb_arcs;
    gr_file >> nb_nodes >> nb_arcs;
    weights.resize(0);
    weights.reserve(nb_arcs);

    int from, to;
    while(gr_file >> from >> to) {
        arcs.emplace_back(from, to);
        weights.emplace_back(1);
    }
    
    graph = graph_t(edges_are_unsorted_multi_pass, arcs.begin(), arcs.end(),
                    weights.data(), nb_nodes);
}

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        graph_t graph;
        std::vector<Edge_Cost> length_map;
        parse_gr(gr_file, graph, length_map);

        const int nb_nodes = num_vertices(graph);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_iterations = 30000.0 * 10000.0 / nb_nodes;
        graph_traits<graph_t>::vertex_iterator si, send;
        for(tie(si, send) = vertices(graph); si != send; ++si) {
            std::vector<vertex_descriptor> p(nb_nodes);
            std::vector<int> d(nb_nodes);
            vertex_descriptor s = *si;

            Chrono chrono;
            int sum = 0;
            dijkstra_shortest_paths_no_color_map(
                graph, s,
                predecessor_map(&p[0]).distance_map(&d[0]).weight_map(
                    get(&Edge_Cost::weight, graph)));

            graph_traits<graph_t>::vertex_iterator vi, vend;
            for(tie(vi, vend) = vertices(graph); vi != vend; ++vi) {
                sum += d[*vi];
            }

            double time_ms = (chrono.timeUs() / 1000.0);
            avg_time += time_ms;
            ++iterations;
            if(iterations >= nb_iterations) break;
        }
        avg_time /= iterations;

        std::cout << gr_file.stem() << ',' << nb_nodes << ','
                  << num_edges(graph) << ',' << avg_time << std::endl;
    }
    return 0;
}
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>

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

class my_visitor : public boost::default_bfs_visitor {
public:
    int & sum;
    my_visitor(int & sum) : sum(sum) {}
    void discover_vertex(const graph_t::vertex_descriptor & s, const graph_t & g) {
        (void)g;
        sum += s;
    }
};

int main() {
    std::vector<std::filesystem::path> gr_files(
        {"data/web-Stanford.txt", "data/Amazon0505.txt", "data/WikiTalk.txt"});

    std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    (void)warm_up();

    for(const auto & gr_file : gr_files) {
        graph_t graph;
        parse_gr(gr_file, graph);

        const int nb_nodes = num_vertices(graph);

        Chrono gr_chrono;
        double avg_time = 0;
        int iterations = 0;
        const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
        graph_traits<graph_t>::vertex_iterator si, send;
        for(tie(si, send) = vertices(graph); si != send; ++si) {
            vertex_descriptor s = *si;

            Chrono chrono;
            int sum = 0;

            my_visitor vis(sum);
            boost::breadth_first_search(graph, s, boost::visitor(vis));

            (void)vis.sum;

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
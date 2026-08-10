#include <iostream>
#include <utility>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/strong_components.hpp>

using namespace boost;

// PROOF THAT boost::connected_components ON DIRECTED GRAPHS IS WRONG !

int main(int argc, char ** argv) {
    using graph_t =
        boost::compressed_sparse_row_graph<boost::directedS, boost::no_property,
                                           boost::no_property>;

    std::vector<std::pair<int, int>> arcs;

    const int nb_nodes = 4;
    arcs.emplace_back(0, 1);
    arcs.emplace_back(2, 1);

    std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
        if(a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    });
    graph_t graph(boost::edges_are_sorted, arcs.begin(), arcs.end(), nb_nodes);

    std::vector<int> compMap(nb_nodes);
    int nb_components = boost::connected_components(
        graph, make_iterator_property_map(
                   compMap.begin(), boost::get(boost::vertex_index, graph)));

    for(auto && u : {0, 1, 2, 3}) {
        std::cout << u << " " << compMap[u] << std::endl;
    }
}

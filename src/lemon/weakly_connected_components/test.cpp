#include <iostream>

#include <lemon/connectivity.h>
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

using namespace lemon;

using _Graph = ListDigraph;

// PROOF THAT lemon::connectedComponents ON DIRECTED GRAPHS IS WRONG !

int main(int argc, char ** argv) {
    _Graph graph;

    auto a = graph.addNode();
    auto b = graph.addNode();
    auto c = graph.addNode();
    auto d = graph.addNode();

    graph.addArc(a, b);
    graph.addArc(c, b);

    typename _Graph::NodeMap<int> compMap(graph);
    lemon::connectedComponents(graph, compMap);

    for(auto && u : {a, b, c, d})
        std::cout << graph.id(u) << " " << compMap[u] << std::endl;
}

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "chrono.hpp"

#include "CXXGraph.hpp"
#include "warm_up.hpp"

// auto parse_gr(const std::filesystem::path & file_name) {
//     static_digraph_builder<double> builder(0);

//     std::ifstream gr_file(file_name);
//     std::string line;
//     while(getline(gr_file, line)) {
//         std::istringstream iss(line);
//         char ch;
//         if(iss >> ch) {
//             switch(ch) {
//                 case 'c':
//                     break;
//                 case 'p': {
//                     std::string format;
//                     std::size_t nb_nodes, nb_arcs;
//                     if(iss >> format >> nb_nodes >> nb_arcs) {
//                         builder = static_digraph_builder<double>(nb_nodes);
//                     }
//                     break;
//                 }
//                 case 'a': {
//                     static_digraph::vertex from, to;
//                     double length;
//                     if(iss >> from >> to >> length) {
//                         builder.add_arc(from - 1, to - 1, length);
//                     }
//                     break;
//                 }
//                 default:
//                     std::cerr << "Error in reading " << file_name << std::endl;
//                     std::abort();
//             }
//         }
//     }

//     return builder.build();
// }

int main() {
    // std::vector<std::filesystem::path> gr_files(
    //     {"data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr",
    //      "data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr",
    //      "data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr",
    //      "data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr",
    //      "data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr",
    //      "data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr",
    //      "data/9th_DIMACS_USA_roads/distance/USA-road-d.FLA.gr",
    //      "data/9th_DIMACS_USA_roads/time/USA-road-t.FLA.gr",
    //      "data/9th_DIMACS_USA_roads/distance/USA-road-d.NW.gr",
    //      "data/9th_DIMACS_USA_roads/time/USA-road-t.NW.gr",
    //      "data/9th_DIMACS_USA_roads/distance/USA-road-d.NE.gr",
    //      "data/9th_DIMACS_USA_roads/time/USA-road-t.NE.gr"});

    // std::cout << "instance,nb_nodes,nb_arcs,time_ms\n";

    // (void)warm_up();

    // for(const auto & gr_file : gr_files) {
    //     auto [graph, length_map] = parse_gr(gr_file);

    //     Chrono gr_chrono;
    //     double avg_time = 0;
    //     int iterations = 0;
    //     const int nb_nodes = graph.nb_vertices();
    //     const int nb_iterations = 30000.0 * 1000.0 / nb_nodes;
    //     for(auto && s : graph.vertices()) {
    //         Chrono chrono;

    //         double sum = 0;
    //         Dijkstra dijkstra(graph, length_map);
    //         dijkstra.add_source(s);
            
    //         for(auto && [u, dist] : dijkstra) {
    //             sum += dist;
    //         }

    //         double time_ms = (chrono.timeUs() / 1000.0);
    //         avg_time += time_ms;
    //         ++iterations;
    //         if(iterations >= nb_iterations) break;
    //     }
    //     avg_time /= iterations;

    //     std::cout << gr_file.stem() << ',' << nb_nodes << ',' << graph.nb_arcs()
    //               << ',' << avg_time << std::endl;
    // }
    // return 0;

    CXXGRAPH::Node<int> node0("0", 0);
    CXXGRAPH::Node<int> node1("1", 1);
    CXXGRAPH::Node<int> node2("2", 2);
    CXXGRAPH::Node<int> node3("3", 3);

    CXXGRAPH::DirectedWeightedEdge<int> edge1(1, node1, node2, 2.0);
    CXXGRAPH::UndirectedWeightedEdge<int> edge2(2, node2, node3, 2.0);
    CXXGRAPH::UndirectedWeightedEdge<int> edge3(3, node0, node1, 2.0);
    CXXGRAPH::UndirectedWeightedEdge<int> edge4(4, node0, node3, 1.0);


    std::set<const CXXGRAPH::Edge<int> *> edgeSet;
    edgeSet.insert(&edge1);
    edgeSet.insert(&edge2);
    edgeSet.insert(&edge3);
    edgeSet.insert(&edge4);

    // Can print out the edges for debugging
    std::cout << edge1 << "\n";
    std::cout << edge2 << "\n";
    std::cout << edge3 << "\n";
    std::cout << edge4 << "\n";

    CXXGRAPH::Graph<int> graph(edgeSet);
    auto res = graph.dijkstra(node0, node2);

    std::cout << "Dijkstra Result: " << res.result << "\n";

    return 0;
}
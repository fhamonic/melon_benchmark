#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/edmonds_karp_max_flow.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/read_dimacs.hpp>

using namespace boost;

#include "chrono.hpp"
#include "warm_up.hpp"

// typedef compressed_sparse_row_graph<directedS, no_property, Edge_Cost>
// graph_t; typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
// typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;
// typedef std::pair<int, int> Edge;

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
        typedef adjacency_list_traits<vecS, vecS, directedS> Traits;
        typedef adjacency_list<
            listS, vecS, directedS, property<vertex_name_t, std::string>,
            property<
                edge_capacity_t, int,
                property<edge_residual_capacity_t, int,
                         property<edge_reverse_t, Traits::edge_descriptor>>>>
            Graph;

        Graph g;

        property_map<Graph, edge_capacity_t>::type capacity =
            get(edge_capacity, g);
        property_map<Graph, edge_reverse_t>::type rev = get(edge_reverse, g);
        property_map<Graph, edge_residual_capacity_t>::type residual_capacity =
            get(edge_residual_capacity, g);

        Traits::vertex_descriptor s, t;
        std::ifstream dimacs(gr_file);
            read_dimacs_max_flow(g, capacity, rev, s, t, dimacs);

        Chrono chrono;
        int flow = edmonds_karp_max_flow(g, s, t);

        std::cout << gr_file.stem() << ',' << num_vertices(g) << ','
                  << num_edges(g) << ',' << (chrono.timeUs() / 1000.0)
                  << std::endl;
    }
    return 0;
}
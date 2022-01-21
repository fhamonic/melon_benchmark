#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "chrono.hpp"

typedef adjacency_list<vecS, vecS, directedS, no_property,
                       property<edge_weight_t, double>>
    graph_t;
typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;
typedef std::pair<int, int> Edge;

void parse_gr(std::string file_name, graph_t & graph,
              property_map<graph_t, edge_weight_t>::type & length_map) {
    int nb_nodes;
    std::vector<std::tuple<int, int, double>> arcs;

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
                    std::string format;
                    int nb_arcs;
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

    graph = graph_t(nb_nodes);
    length_map = get(edge_weight, graph);
    for(const auto & [u, v, length] : arcs) {
        edge_descriptor e;
        bool inserted;
        tie(e, inserted) = add_edge(u, v, graph);
        length_map[e] = length;
    }
}

int main() {
    std::vector<std::string> gr_files({
        // "data/rome99.gr",
        "data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr",
         "data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr",
         "data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr"
    });

    int rows = 0;
    for(const auto & gr_file : gr_files) {
        graph_t graph;
        property_map<graph_t, edge_weight_t>::type length_map;
        parse_gr(gr_file, graph, length_map);

        std::cout << std::setprecision(16) << gr_file << " : " << num_vertices(graph) << " nodes , "
                  << num_edges(graph) << " arcs" << std::endl;

        graph_traits<graph_t>::vertex_iterator si, send;
        for(tie(si, send) = vertices(graph); si != send; ++si) {
            std::vector<vertex_descriptor> p(num_vertices(graph));
            std::vector<int> d(num_vertices(graph));
            vertex_descriptor s = *si;

            Chrono chrono;
            dijkstra_shortest_paths(graph, s,
                                    predecessor_map(&p[0]).distance_map(&d[0]));

            graph_traits<graph_t>::vertex_iterator vi, vend;
            for(tie(vi, vend) = vertices(graph); vi != vend; ++vi) {
                std::cout << s << ',' << *vi << ':' << d[*vi] << '\n';
                ++rows;
            }
            if(rows > 20000000) goto finish;
        }
    }
finish:
    std::cout << std::endl;
    return 0;
}
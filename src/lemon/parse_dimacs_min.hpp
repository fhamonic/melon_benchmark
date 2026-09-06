#pragma once

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "helper.hpp"
#include "input_file.hpp"

// DIMACS min-cost-flow format ("p min <n> <m>"): "n <id> <supply>" for the
// nodes with a non-zero supply (positive) or demand (negative), then
// "a <u> <v> <lower> <upper> <cost>" per arc. Unlisted nodes are
// transshipment nodes.
//
// Lower bounds are read and required to be zero rather than ignored: neither
// benchmarked implementation is given a lower-bound map here, so an instance
// carrying non-zero lower bounds would be silently solved as a different --
// easier -- problem by everything in the chart.
//
// The whole file is buffered before the graph is built. StaticDigraph::build
// needs the arcs up front and in source order, and the supply lines name node
// ids that do not exist until it has run.
template <typename _Graph, typename _Value>
void parse_dimacs_min(const std::filesystem::path & file_name, _Graph & graph,
                      typename _Graph::template ArcMap<_Value> & capacity_map,
                      typename _Graph::template ArcMap<_Value> & cost_map,
                      typename _Graph::template NodeMap<_Value> & supply_map) {
    struct arc_entry {
        int first;
        int second;
        _Value capacity;
        _Value cost;
    };

    auto min_file = open_input_file(file_name);
    int nb_nodes = 0, nb_arcs = 0;
    std::vector<arc_entry> arcs;
    std::vector<std::pair<int, _Value>> supplies;

    std::string line;
    std::size_t line_no = 0;
    while(std::getline(min_file, line)) {
        ++line_no;
        std::istringstream iss(line);
        char ch;
        if(!(iss >> ch)) continue;
        switch(ch) {
            case 'c':
                break;
            case 'p': {
                std::string format;
                if(iss >> format >> nb_nodes >> nb_arcs)
                    arcs.reserve(static_cast<std::size_t>(nb_arcs));
                break;
            }
            case 'n': {
                int id;
                _Value supply;
                if(iss >> id >> supply) supplies.emplace_back(id - 1, supply);
                break;
            }
            case 'a': {
                int from, to;
                _Value lower, upper, cost;
                if(iss >> from >> to >> lower >> upper >> cost) {
                    if(lower != _Value{0}) {
                        std::cerr << "error in reading " << file_name << ":"
                                  << line_no
                                  << ": non-zero lower bound, which the"
                                     " benchmarks do not model\n";
                        std::exit(1);
                    }
                    arcs.emplace_back(from - 1, to - 1, upper, cost);
                }
                break;
            }
            default:
                std::cerr << "Error in reading " << file_name << " unknown '"
                          << ch << '\'' << std::endl;
                std::abort();
        }
    }

    if constexpr(std::same_as<_Graph, StaticDigraph>) {
        std::sort(arcs.begin(), arcs.end(), [](const auto & a, const auto & b) {
            if(a.first == b.first) return a.second < b.second;
            return a.first < b.first;
        });
        graph.build(nb_nodes, arcs.begin(), arcs.end());
        for(std::size_t i = 0; i < arcs.size(); ++i) {
            const auto a = graph.arcFromId(static_cast<int>(i));
            capacity_map[a] = arcs[i].capacity;
            cost_map[a] = arcs[i].cost;
        }
    } else {
        for(int i = 0; i < nb_nodes; ++i) graph.addNode();
        for(const auto & e : arcs) {
            const auto a = graph.addArc(graph.nodeFromId(e.first),
                                        graph.nodeFromId(e.second));
            capacity_map[a] = e.capacity;
            cost_map[a] = e.cost;
        }
    }

    for(int i = 0; i < nb_nodes; ++i)
        supply_map[graph.nodeFromId(i)] = _Value{0};
    for(const auto & [id, supply] : supplies)
        supply_map[graph.nodeFromId(id)] = supply;
}

// See cached_parse_dimacs.
template <typename _Graph, typename _Value>
struct dimacs_min_instance {
    _Graph graph;
    typename _Graph::template ArcMap<_Value> capacities{graph};
    typename _Graph::template ArcMap<_Value> costs{graph};
    typename _Graph::template NodeMap<_Value> supplies{graph};
};

template <typename _Graph, typename _Value>
[[nodiscard]] dimacs_min_instance<_Graph, _Value> & cached_parse_dimacs_min(
    const std::filesystem::path & min_file) {
    return cached_parse_into<dimacs_min_instance<_Graph, _Value>>(
        min_file, [&](auto & instance) {
            parse_dimacs_min<_Graph, _Value>(min_file, instance.graph,
                                             instance.capacities,
                                             instance.costs, instance.supplies);
        });
}

#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "input_file.hpp"

#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"
#include "melon/graph.hpp"
#include "melon/utility/static_digraph_builder.hpp"

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
// Returns (graph, capacities, costs, supplies). Supplies are indexed by
// vertex id and capacities and costs by arc, which is what melon's mapping
// views over a std::vector give.
template <typename _Graph, typename _Value>
auto parse_dimacs_min(const std::filesystem::path & min_file) {
    auto file = open_input_file(min_file);

    std::size_t nb_nodes = 0, nb_arcs = 0;
    std::vector<_Value> supplies;
    // Arcs are buffered rather than fed to the graph as they are read: a
    // static_digraph_builder wants its vertex count up front, and buffering
    // is the one shape that serves both containers.
    struct arc_entry {
        std::size_t source, target;
        _Value capacity;
        _Value cost;
    };
    std::vector<arc_entry> arcs;

    std::string line;
    std::size_t line_no = 0;
    while(std::getline(file, line)) {
        ++line_no;
        std::istringstream iss(line);
        char ch;
        if(!(iss >> ch)) continue;
        switch(ch) {
            case 'c':
                break;
            case 'p': {
                std::string format;
                if(iss >> format >> nb_nodes >> nb_arcs) {
                    supplies.assign(nb_nodes, _Value{0});
                    arcs.reserve(nb_arcs);
                }
                break;
            }
            case 'n': {
                std::size_t id;
                _Value supply;
                if(iss >> id >> supply) supplies[id - 1] = supply;
                break;
            }
            case 'a': {
                std::size_t from, to;
                _Value lower, upper, cost;
                if(iss >> from >> to >> lower >> upper >> cost) {
                    if(lower != _Value{0}) {
                        std::cerr << "error in reading " << min_file << ":"
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
                std::cerr << "error in reading " << min_file << ":" << line_no
                          << " unknown '" << ch << "'\n";
                std::abort();
        }
    }

    if constexpr(std::same_as<_Graph, melon::mutable_digraph>) {
        melon::mutable_digraph graph;
        for(std::size_t i = 0; i < nb_nodes; ++i) (void)graph.create_vertex();
        std::vector<_Value> capacities(arcs.size()), costs(arcs.size());
        for(const auto & e : arcs) {
            const auto a = graph.create_arc(
                static_cast<melon::vertex_t<melon::mutable_digraph>>(e.source),
                static_cast<melon::vertex_t<melon::mutable_digraph>>(e.target));
            capacities[a] = e.capacity;
            costs[a] = e.cost;
        }
        return std::make_tuple(std::move(graph), std::move(capacities),
                               std::move(costs), std::move(supplies));
    } else {
        melon::static_digraph_builder<_Graph, _Value, _Value> builder(nb_nodes);
        for(const auto & e : arcs)
            builder.add_arc({static_cast<melon::vertex_t<_Graph>>(e.source),
                            static_cast<melon::vertex_t<_Graph>>(e.target)},
                            e.capacity, e.cost);
        auto [graph, capacities, costs] = std::move(builder).build();
        return std::make_tuple(std::move(graph), std::move(capacities),
                               std::move(costs), std::move(supplies));
    }
}

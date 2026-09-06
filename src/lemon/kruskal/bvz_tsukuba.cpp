#include <cmath>
#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/kruskal.h>

using namespace lemon;

#include "bvz_tsukuba_instances.hpp"
#include "checksum.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph, typename _Value>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        auto & [graph, costs] =
            cached_parse_undirected_dimacs<_Graph, _Value>(gr_file);

        state.SetLabel(cached_setup(gr_file, [&] {
            typename _Graph::EdgeMap<bool> tree_map(graph);
            kruskal(graph, costs, tree_map);
            std::size_t num_tree_edges = 0;
            _Value total = 0;
            for(typename _Graph::EdgeIt e(graph); e != INVALID; ++e) {
                if(!tree_map[e]) continue;
                ++num_tree_edges;
                total += costs[e];
            }
            checksum cs;
            cs.add(num_tree_edges);
            cs.add(total);
            return cs.str();
        }));

        for(auto _ : state) {
            typename _Graph::EdgeMap<bool> tree_map(graph);
            auto algo = kruskal(graph, costs, tree_map);
            for(typename _Graph::EdgeIt e(graph); e != INVALID; ++e) {
                benchmark::DoNotOptimize(tree_map[e]);
            }
        }
    }
};

#define REGISTER(graph, value)                                       \
    benchmark::RegisterBenchmark(                                    \
        std::string(gr_file.stem().c_str()) + "/" #graph "/" #value, \
        BM<graph, value>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(SmartGraph, int)
        REGISTER(ListGraph, int)
        REGISTER(SmartGraph, double)
        REGISTER(ListGraph, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

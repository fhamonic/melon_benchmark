#include <cmath>
#include <filesystem>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/bellman_ford.h>
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

using namespace lemon;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

// Storage policy matches the other two libraries: a distance map and a
// predecessor map, both allocated per query -- LEMON's default traits.
//
// checkedStart(), not start(): the suite's policy is negative-cycle detection
// everywhere (Boost's bellman_ford_shortest_paths cannot even turn its
// certification pass off). For LEMON detection is only the emptiness of the
// process list, so this costs at most one extra round.
template <typename _Graph, typename _Value>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] =
            cached_parse_dimacs<_Graph, _Value>(gr_file);

        state.SetLabel(cached_setup(gr_file, [&] {
            checksum cs;
            for(auto && s : sources) {
                BellmanFord<_Graph, typename _Graph::ArcMap<_Value>> algo(
                    graph, length_map);
                algo.init();
                algo.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));
                algo.checkedStart();
                lemon_add_distances(
                    graph, cs, [&](const auto & u) { return algo.reached(u); },
                    [&](const auto & u) { return algo.dist(u); });
            }
            return cs.str();
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                BellmanFord<_Graph, typename _Graph::ArcMap<_Value>> algo(
                    graph, length_map);
                algo.init();
                algo.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));
                benchmark::DoNotOptimize(algo.checkedStart());
                // The O(n) read-back stays inside the timed region because the
                // MELON and Boost loops pay the same sweep.
                const int n = countNodes(graph);
                for(int i = 0; i < n; ++i) {
                    benchmark::DoNotOptimize(algo.dist(graph.nodeFromId(i)));
                }
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER(graph, value)                                       \
    benchmark::RegisterBenchmark(                                    \
        std::string(gr_file.stem().c_str()) + "/" #graph "/" #value, \
        BM<graph, value>{}, gr_file, sources);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    // Bellman-Ford's cost model is n*m, not Dijkstra's (n+m)*log(n). Getting
    // that wrong understated the work by five orders of magnitude on the
    // larger road networks, where one run takes tens of seconds; it is also
    // what decides whether an instance is worth running at all.
    const auto complexity = [](int n, int m) {
        return static_cast<double>(n) * static_cast<double>(m);
    };
    for(const auto & [gr_file, num_vertices, num_arcs] : instances) {
        if(!instance_is_affordable(complexity(num_vertices, num_arcs)))
            continue;
        const auto sources =
            instance_sources(num_vertices, num_arcs, complexity);
        REGISTER(StaticDigraph, int)
        REGISTER(StaticDigraph, double)
        REGISTER(SmartDigraph, int)
        REGISTER(SmartDigraph, double)
        REGISTER(ListDigraph, int)
        REGISTER(ListDigraph, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

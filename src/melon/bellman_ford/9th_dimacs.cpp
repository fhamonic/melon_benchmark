#include <cmath>
#include <filesystem>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

#include "melon/algorithm/bellman_ford.hpp"

using namespace melon;

// Storage policy is fixed across all three libraries: each keeps a distance
// map and a predecessor map, because that is what LEMON and Boost allocate by
// default and the comparison is meaningless if only MELON is allowed to skip
// the bookkeeping. MELON is faster with both disabled -- that is a genuine
// advantage of its traits, but measuring it here would not be a comparison.
template <typename _Graph, typename _Value>
struct bench_bellman_ford_traits {
    using semiring = shortest_path_semiring<_Value>;
    static constexpr bool store_paths = true;
    static constexpr bool detect_negative_cycles = true;
};

template <typename _Graph, typename _Value>
struct BM {
    using traits = bench_bellman_ford_traits<_Graph, _Value>;

    // Distance of every vertex, in vertex id order, for every source.
    static std::string result_checksum(
        const _Graph & graph, const auto & length_map,
        const std::vector<unsigned int> & sources) {
        // Iterate vertex *ids*, not melon::vertices(graph): mutable_digraph
        // walks an intrusive list and yields them in reverse creation order,
        // which would make the digest container-dependent.
        const std::size_t n = melon::num_vertices(graph);
        checksum cs;
        for(auto && s : sources) {
            auto algo = bellman_ford(traits{}, graph, length_map, s);
            algo.run();
            for(std::size_t i = 0; i < n; ++i) {
                const auto u = static_cast<melon::vertex_t<_Graph>>(i);
                if(algo.reached(u))
                    cs.add(algo.dist(u));
                else
                    cs.add_unreached();
            }
        }
        return cs.str();
    }

    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_dimacs<_Graph, _Value>(gr_file); });

        state.SetLabel(cached_setup(gr_file, [&] {
            return result_checksum(graph, length_map, sources);
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                auto algo = bellman_ford(traits{}, graph, length_map, s);
                algo.run();
                for(auto && u : melon::vertices(graph)) {
                    benchmark::DoNotOptimize(algo.dist(u));
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
        REGISTER(static_digraph, int)
        REGISTER(static_digraph, double)
        REGISTER(mutable_digraph, int)
        REGISTER(mutable_digraph, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

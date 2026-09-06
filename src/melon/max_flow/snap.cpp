// Max-flow on the SNAP graphs, unit capacities, between vertices 0 and 1.
// SNAP ships no reference solution, so the digest is the only cross-check:
// every container and every algorithm must agree on the flow value.

#include <filesystem>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/algorithm/dinitz.hpp"
#include "melon/algorithm/edmonds_karp.hpp"
#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"

using namespace melon;

#include "checksum.hpp"
#include "parse_snap.hpp"
#include "snap_instances.hpp"

template <typename _Graph, typename _Make>
void run_max_flow(benchmark::State & state,
                  const std::filesystem::path & gr_file, _Make && make) {
    auto & graph =
        cached_parse(gr_file, [&] { return parse_snap<_Graph>(gr_file); });
    const auto unit_capacity = [](auto &&) { return 1; };

    state.SetLabel(cached_setup(gr_file, [&] {
        auto algo = make(graph, unit_capacity, 0u, 1u);
        algo.run();
        checksum cs;
        cs.add(algo.flow_value());
        return cs.str();
    }));

    for(auto _ : state) {
        auto algo = make(graph, unit_capacity, 0u, 1u);
        algo.run();
        benchmark::DoNotOptimize(algo.flow_value());
    }
}

template <typename _Graph>
struct BM_dinitz {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Graph>(state, gr_file,
                             [](auto && g, auto && c, auto && s, auto && t) {
                                 return dinitz(g, c, s, t);
                             });
    }
};

template <typename _Graph>
struct BM_edmonds_karp {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Graph>(state, gr_file,
                             [](auto && g, auto && c, auto && s, auto && t) {
                                 return edmonds_karp(g, c, s, t);
                             });
    }
};

#define REGISTER(algo, graph)                                            \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +   \
                                     "/" #graph ":" #algo "/unweighted", \
                                 BM_##algo<graph>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(dinitz, static_digraph)
        REGISTER(dinitz, mutable_digraph)
        // Edmonds-Karp is disabled: its O(V.E^2) augmenting-path bound makes
        // it one to two orders of magnitude slower than every other max-flow
        // algorithm here -- tens of seconds per BVZ-tsukuba instance against
        // Dinitz's ~0.3 s -- so it dominates the runtime of a full `make`
        // while telling nobody anything they did not already know. The code
        // is kept so it can be re-enabled when it is actually wanted.
        // REGISTER(edmonds_karp, static_digraph)
        // REGISTER(edmonds_karp, mutable_digraph)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

// Dataset-independent body: the including .cpp provides the `instances`
// list (a <dataset>_instances.hpp include) before including this header.

// Max-flow benchmarks.
//
// The graph dimension of each registered name is "<container>:<algorithm>"
// rather than just "<container>", so that every library's max-flow algorithms
// land in the same chart. Dinitz has no counterpart in LEMON or Boost, and a
// chart with a single library in it is not a benchmark -- what is comparable
// is "the best max-flow each library offers on this instance".
//
// Edmonds-Karp is present but disabled; see the note in main().

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
#include "helper.hpp"
#include "max_flow_reference.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph, typename _Value, typename _Make>
void run_max_flow(benchmark::State & state,
                  const std::filesystem::path & gr_file, _Make && make) {
    auto & [graph, capacities] = cached_parse(
        gr_file, [&] { return parse_dimacs<_Graph, _Value>(gr_file); });
    // Terminals come from the file's "n <id> s" / "n <id> t" lines rather than
    // being assumed to be vertices 0 and 1.
    const auto [source_id, sink_id] = max_flow_terminals(gr_file);
    const auto s = static_cast<vertex_t<_Graph>>(source_id);
    const auto t = static_cast<vertex_t<_Graph>>(sink_id);

    // The reference check is a property of the instance, not of the
    // repetition, so it is cached together with the digest it guards.
    struct verified {
        std::string label;
        std::string error;
    };
    const auto & result = cached_setup(gr_file, [&] {
        auto algo = make(graph, capacities, s, t);
        algo.run();
        const auto flow = algo.flow_value();

        // BVZ-tsukuba ships the optimal flow value next to each instance. A
        // max-flow benchmark that does not check it is timing an unknown
        // computation.
        if(const auto expected = reference_flow_value(gr_file)) {
            if(static_cast<long long>(flow) != *expected)
                return verified{{},
                                "flow value " + std::to_string(flow) +
                                    " != reference " +
                                    std::to_string(*expected)};
        }
        checksum cs;
        cs.add(flow);
        return verified{cs.str(), {}};
    });
    if(!result.error.empty()) {
        state.SkipWithError(result.error);
        return;
    }
    state.SetLabel(result.label);

    for(auto _ : state) {
        auto algo = make(graph, capacities, s, t);
        algo.run();
        benchmark::DoNotOptimize(algo.flow_value());
    }
}

template <typename _Graph, typename _Value>
struct BM_dinitz {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Graph, _Value>(
            state, gr_file, [](auto && g, auto && c, auto && s, auto && t) {
                return dinitz(g, c, s, t);
            });
    }
};

template <typename _Graph, typename _Value>
struct BM_edmonds_karp {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Graph, _Value>(
            state, gr_file, [](auto && g, auto && c, auto && s, auto && t) {
                return edmonds_karp(g, c, s, t);
            });
    }
};

#define REGISTER(algo, graph, value)                                           \
    benchmark::RegisterBenchmark(                                              \
        std::string(gr_file.stem().c_str()) + "/" #graph ":" #algo "/" #value, \
        BM_##algo<graph, value>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(dinitz, static_digraph, int)
        REGISTER(dinitz, mutable_digraph, int)
        REGISTER(dinitz, static_digraph, double)
        REGISTER(dinitz, mutable_digraph, double)
        // Edmonds-Karp is disabled: its O(V.E^2) augmenting-path bound makes
        // it one to two orders of magnitude slower than every other max-flow
        // algorithm here -- tens of seconds per BVZ-tsukuba instance against
        // Dinitz's ~0.3 s -- so it dominates the runtime of a full `make`
        // while telling nobody anything they did not already know. The code
        // is kept so it can be re-enabled when it is actually wanted.
        // REGISTER(edmonds_karp, static_digraph, int)
        // REGISTER(edmonds_karp, mutable_digraph, int)
        // REGISTER(edmonds_karp, static_digraph, double)
        // REGISTER(edmonds_karp, mutable_digraph, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

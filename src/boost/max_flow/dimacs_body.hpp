// Dataset-independent body: the including .cpp provides the `instances`
// list (a <dataset>_instances.hpp include) before including this header.

// Boost max-flow baselines for MELON's Dinitz.
//
// Boost has no Dinitz either. push_relabel is its fast general max-flow, and
// boykov_kolmogorov is the algorithm BVZ-tsukuba was published to benchmark in
// the first place, so both belong here. edmonds_karp is present but disabled;
// see the note in main().

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boykov_kolmogorov_max_flow.hpp>
#include <boost/graph/edmonds_karp_max_flow.hpp>
#include <boost/graph/push_relabel_max_flow.hpp>

using namespace boost;

#include "checksum.hpp"
#include "helper.hpp"
#include "max_flow_reference.hpp"
#include "parse_max_flow.hpp"

template <typename _Value, typename _Run>
void run_max_flow(benchmark::State & state,
                  const std::filesystem::path & gr_file, _Run && run) {
    auto & graph = cached_parse(
        gr_file, [&] { return parse_max_flow_dimacs<_Value>(gr_file); });
    const auto [source_id, sink_id] = max_flow_terminals(gr_file);
    const auto s = vertex(source_id, graph);
    const auto t = vertex(sink_id, graph);

    // The reference check is a property of the instance, not of the
    // repetition, so it is cached together with the digest it guards.
    struct verified {
        std::string label;
        std::string error;
    };
    const auto & result = cached_setup(gr_file, [&] {
        const _Value flow = run(graph, s, t);
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
        benchmark::DoNotOptimize(run(graph, s, t));
    }
}

template <typename _Value>
struct BM_push_relabel {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Value>(state, gr_file, [](auto & g, auto s, auto t) {
            return push_relabel_max_flow(g, s, t);
        });
    }
};

// boykov_kolmogorov needs predecessor, colour and distance maps. Its
// convenience overload looks for them as *internal* vertex properties, which
// would mean declaring them on the graph type and making push_relabel and
// edmonds_karp carry vertex state they never touch. External maps instead,
// allocated per run like every other algorithm's working state.
template <typename _Value>
struct BM_boykov_kolmogorov {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Value>(state, gr_file, [](auto & g, auto s, auto t) {
            using graph_t = std::decay_t<decltype(g)>;
            using edge_descriptor =
                typename graph_traits<graph_t>::edge_descriptor;
            const std::size_t n = num_vertices(g);
            std::vector<edge_descriptor> predecessors(n);
            std::vector<default_color_type> colors(n);
            std::vector<long> distances(n);
            return boykov_kolmogorov_max_flow(
                g, get(edge_capacity, g), get(edge_residual_capacity, g),
                get(edge_reverse, g), predecessors.data(), colors.data(),
                distances.data(), get(vertex_index, g), s, t);
        });
    }
};

template <typename _Value>
struct BM_edmonds_karp {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        run_max_flow<_Value>(state, gr_file, [](auto & g, auto s, auto t) {
            return edmonds_karp_max_flow(g, s, t);
        });
    }
};

#define REGISTER(algo, value)                                             \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +    \
                                     "/adjacency_list:" #algo "/" #value, \
                                 BM_##algo<value>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(push_relabel, int)
        REGISTER(boykov_kolmogorov, int)
        // Edmonds-Karp is disabled: its O(V.E^2) augmenting-path bound makes
        // it one to two orders of magnitude slower than every other max-flow
        // algorithm here -- tens of seconds per BVZ-tsukuba instance against
        // Dinitz's ~0.3 s -- so it dominates the runtime of a full `make`
        // while telling nobody anything they did not already know. The code
        // is kept so it can be re-enabled when it is actually wanted.
        // REGISTER(edmonds_karp, int)
        REGISTER(push_relabel, double)
        REGISTER(boykov_kolmogorov, double)
        // REGISTER(edmonds_karp, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

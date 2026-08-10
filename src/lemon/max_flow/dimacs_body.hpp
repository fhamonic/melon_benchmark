// Dataset-independent body: the including .cpp provides the `instances`
// list (a <dataset>_instances.hpp include) before including this header.

// LEMON max-flow baselines for MELON's Dinitz.
//
// LEMON has no Dinitz. Preflow (push-relabel with highest-label selection) is
// what it offers as its fast max-flow, so that is the honest counterpart.
// EdmondsKarp is present but disabled; see the note in main().

#include <filesystem>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/edmonds_karp.h>
#include <lemon/preflow.h>

using namespace lemon;

#include "checksum.hpp"
#include "max_flow_reference.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph, typename _Value,
          template <typename, typename> class _Algorithm>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        using capacity_map = typename _Graph::template ArcMap<_Value>;
        using algorithm = _Algorithm<_Graph, capacity_map>;

        _Graph graph;
        capacity_map capacities(graph);
        parse_dimacs<_Graph, _Value>(gr_file, graph, capacities);

        const auto [source_id, sink_id] = max_flow_terminals(gr_file);
        const auto s =
            graph.fromId(static_cast<int>(source_id), typename _Graph::Node());
        const auto t =
            graph.fromId(static_cast<int>(sink_id), typename _Graph::Node());

        {
            algorithm algo(graph, capacities, s, t);
            algo.run();
            const auto flow = algo.flowValue();
            if(const auto expected = reference_flow_value(gr_file)) {
                if(static_cast<long long>(flow) != *expected) {
                    state.SkipWithError("flow value " + std::to_string(flow) +
                                        " != reference " +
                                        std::to_string(*expected));
                    return;
                }
            }
            checksum cs;
            cs.add(flow);
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            algorithm algo(graph, capacities, s, t);
            algo.run();
            benchmark::DoNotOptimize(algo.flowValue());
        }
    }
};

#define REGISTER(algo, algo_name, graph, value)                           \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +    \
                                     "/" #graph ":" algo_name "/" #value, \
                                 BM<graph, value, algo>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(Preflow, "preflow", StaticDigraph, int)
        REGISTER(Preflow, "preflow", SmartDigraph, int)
        REGISTER(Preflow, "preflow", ListDigraph, int)
        REGISTER(Preflow, "preflow", StaticDigraph, double)
        REGISTER(Preflow, "preflow", SmartDigraph, double)
        REGISTER(Preflow, "preflow", ListDigraph, double)
        // Edmonds-Karp is disabled: its O(V.E^2) augmenting-path bound makes
        // it one to two orders of magnitude slower than every other max-flow
        // algorithm here -- tens of seconds per BVZ-tsukuba instance against
        // Dinitz's ~0.3 s -- so it dominates the runtime of a full `make`
        // while telling nobody anything they did not already know. The code
        // is kept so it can be re-enabled when it is actually wanted.
        // REGISTER(EdmondsKarp, "edmonds_karp", StaticDigraph, int)
        // REGISTER(EdmondsKarp, "edmonds_karp", SmartDigraph, int)
        // REGISTER(EdmondsKarp, "edmonds_karp", ListDigraph, int)
        // REGISTER(EdmondsKarp, "edmonds_karp", StaticDigraph, double)
        // REGISTER(EdmondsKarp, "edmonds_karp", SmartDigraph, double)
        // REGISTER(EdmondsKarp, "edmonds_karp", ListDigraph, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

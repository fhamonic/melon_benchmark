// Max-flow on the SNAP graphs, unit capacities, between nodes 0 and 1.
// See src/lemon/max_flow/bvz_tsukuba.cpp for why Preflow is the counterpart
// to MELON's Dinitz.

#include <filesystem>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/edmonds_karp.h>
#include <lemon/preflow.h>

using namespace lemon;

#include "checksum.hpp"
#include "parse_snap.hpp"
#include "snap_instances.hpp"

template <typename _Graph, template <typename, typename> class _Algorithm>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        using capacity_map = ConstMap<typename _Graph::Arc, int>;
        using algorithm = _Algorithm<_Graph, capacity_map>;

        _Graph graph;
        parse_snap(gr_file, graph);
        capacity_map capacities(1);

        const auto s = graph.fromId(0, typename _Graph::Node());
        const auto t = graph.fromId(1, typename _Graph::Node());

        {
            algorithm algo(graph, capacities, s, t);
            algo.run();
            checksum cs;
            cs.add(algo.flowValue());
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            algorithm algo(graph, capacities, s, t);
            algo.run();
            benchmark::DoNotOptimize(algo.flowValue());
        }
    }
};

#define REGISTER(algo, algo_name, graph)                                     \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +       \
                                     "/" #graph ":" algo_name "/unweighted", \
                                 BM<graph, algo>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(Preflow, "preflow", StaticDigraph)
        REGISTER(Preflow, "preflow", SmartDigraph)
        REGISTER(Preflow, "preflow", ListDigraph)
        // Edmonds-Karp is disabled: its O(V.E^2) augmenting-path bound makes
        // it one to two orders of magnitude slower than every other max-flow
        // algorithm here -- tens of seconds per BVZ-tsukuba instance against
        // Dinitz's ~0.3 s -- so it dominates the runtime of a full `make`
        // while telling nobody anything they did not already know. The code
        // is kept so it can be re-enabled when it is actually wanted.
        // REGISTER(EdmondsKarp, "edmonds_karp", StaticDigraph)
        // REGISTER(EdmondsKarp, "edmonds_karp", SmartDigraph)
        // REGISTER(EdmondsKarp, "edmonds_karp", ListDigraph)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

// CXXGraph point-to-point shortest path.
//
// This is the one shortest-path question CXXGraph's API can answer:
// dijkstra(source, target) returns a single distance. It cannot answer the
// single-source question the `dijkstra` benchmark asks, which is why it has no
// bar there.
//
// It also cannot stop early. The implementation runs `while (!pq.empty())` to
// exhaustion and only then reads dist[target], so a point-to-point query costs
// a full single-source computation -- while MELON, LEMON and Boost all stop as
// soon as the target is settled. That difference is the point of this chart:
// it is an API-level cost, not a slow inner loop, and it is exactly the kind of
// thing a benchmark should surface rather than hide.
//
// Registered on rome99 only, for the reasons in breadth_first_search/.

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "CXXGraph/CXXGraph.hpp"

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources,
                    int num_vertices) const {
        auto instance = parse_dimacs<true>(gr_file);

        {
            checksum cs;
            for(auto && s : sources) {
                const auto t = instance_target(s, num_vertices);
                const auto result = instance.graph.dijkstra(*instance.nodes[s],
                                                            *instance.nodes[t]);
                if(result.success)
                    cs.add(result.result);
                else
                    cs.add_unreached();
            }
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            for(auto && s : sources) {
                const auto t = instance_target(s, num_vertices);
                const auto result = instance.graph.dijkstra(*instance.nodes[s],
                                                            *instance.nodes[t]);
                benchmark::DoNotOptimize(result.result);
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        if(gr_file.stem() != "rome99") continue;  // see the note at the top
        const auto sources = instance_sources(
            n, m, [](int n, int m) { return (n + m) * std::log(n); });
        // Registered as "/int" like the other three, because that is the value
        // type of the *problem* -- DIMACS arc lengths are integers -- and the
        // value-type dimension is what puts a run in a chart. CXXGraph
        // computes in double regardless, since DirectedWeightedEdge only
        // stores double; the digests still match because integral lengths and
        // their sums are exact in double.
        benchmark::RegisterBenchmark(
            std::string(gr_file.stem().c_str()) +
                "/shared_ptr_edge_set:unidirectional/int",
            BM{}, gr_file, sources, n);
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

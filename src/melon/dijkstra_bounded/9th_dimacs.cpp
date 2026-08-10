// What does it cost to *keep* the shortest-path tree the caller never asked
// for?
//
// The task is a k-nearest-vertices query: settle the k closest vertices to a
// source, consuming each distance as the vertex comes off the heap, then
// stop. This is the shape of a service-area or k-nearest-neighbour query, and
// it is the shape in which online consumption actually pays.
//
// Every library is registered twice per k, as "<container>:stored" and
// "<container>:streaming", so one chart carries both the cross-library
// comparison and each library's own cost of storage. Only the traits differ
// between the two; the loop body is identical.
//
// k is the variable that matters. Dijkstra's per-query setup is O(|V|)
// regardless of how far the search actually runs, so the maps a library
// cannot avoid allocating are amortized over the search when k is large and
// dominate it when k is small. Registering k from 100 to unbounded shows the
// whole curve rather than one flattering point on it.
//
// Heap arity is fixed at 4 (Boost's default) because the variable under test
// is storage, not the heap.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

#include "melon/algorithm/dijkstra.hpp"

using namespace melon;

static constexpr std::size_t UNBOUNDED =
    std::numeric_limits<std::size_t>::max();

template <typename _Graph, typename _Value, bool _Store>
struct bench_dijkstra_traits {
    using semiring = shortest_path_semiring<_Value>;
    using heap =
        updatable_d_ary_heap<4, std::pair<vertex_t<_Graph>, _Value>,
                             std::less<_Value>,
                             vertex_map_t<_Graph, std::size_t>,
                             maps::element_map<1>, maps::element_map<0>>;

    // The point of the benchmark: MELON lets the caller say "I do not need
    // these". vertex_map_if is then an empty [[no_unique_address]] member --
    // no allocation, no initialization, no write per settled vertex.
    static constexpr bool store_distances = _Store;
    static constexpr bool store_paths = _Store;
};

template <typename _Graph, typename _Value, bool _Store, std::size_t _K>
struct BM {
    using traits = bench_dijkstra_traits<_Graph, _Value, _Store>;

    // The k settled distances, sorted. Sorting makes the digest canonical
    // under ties: which of two equidistant vertices a library settles first
    // is an implementation detail, but the multiset of distances is not.
    static std::string result_checksum(
        const _Graph & graph, const auto & length_map,
        const std::vector<unsigned int> & sources) {
        std::vector<_Value> settled;
        checksum cs;
        for(auto && s : sources) {
            settled.clear();
            for(auto && [u, dist] : dijkstra(traits{}, graph, length_map, s)) {
                settled.push_back(dist);
                if(settled.size() >= _K) break;
            }
            std::sort(settled.begin(), settled.end());
            cs.add(settled.size());
            for(const _Value & d : settled) cs.add(d);
        }
        return cs.str();
    }

    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto [graph, length_map] = parse_dimacs<_Graph, _Value>(gr_file);

        state.SetLabel(result_checksum(graph, length_map, sources));

        // Counted inside the timed region and reported: a reader should not
        // have to take on faith that a 15 us query settled anything. The
        // counter is the number of vertices the *timed* loop actually pulled
        // off the heap, and it must equal min(k, reachable).
        std::size_t total_settled = 0;
        for(auto _ : state) {
            for(auto && s : sources) {
                std::size_t settled = 0;
                _Value total = 0;
                for(auto && [u, dist] :
                    dijkstra(traits{}, graph, length_map, s)) {
                    total += dist;
                    if(++settled >= _K) break;
                }
                benchmark::DoNotOptimize(total);
                total_settled += settled;
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
        state.counters["settled"] =
            static_cast<double>(total_settled) /
            static_cast<double>(state.iterations() * sources.size());
    }
};

#define REGISTER(graph, value, store, mode, k, k_name)                       \
    benchmark::RegisterBenchmark(                                            \
        std::string(gr_file.stem().c_str()) + "/" #graph ":" mode "/" #value \
                                              "/" k_name,                    \
        BM<graph, value, store, k>{}, gr_file, sources);

#define REGISTER_K(k, k_name)                                    \
    REGISTER(static_digraph, int, true, "stored", k, k_name)     \
    REGISTER(static_digraph, int, false, "streaming", k, k_name) \
    REGISTER(mutable_digraph, int, true, "stored", k, k_name)    \
    REGISTER(mutable_digraph, int, false, "streaming", k, k_name)

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, num_vertices, num_arcs] : instances) {
        const auto sources = instance_sources(
            num_vertices, num_arcs,
            [](int n, int m) { return (n + m) * std::log(n); });
        REGISTER_K(100, "k100")
        REGISTER_K(1000, "k1000")
        REGISTER_K(10000, "k10000")
        REGISTER_K(UNBOUNDED, "kall")
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

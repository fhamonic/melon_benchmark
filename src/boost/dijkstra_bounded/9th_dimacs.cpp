// Storage policy across query sizes -- Boost side. See
// src/melon/dijkstra_bounded/9th_dimacs.cpp for the task definition.
//
// Two things Boost cannot do, and both are structural rather than a matter of
// which overload you reach for:
//
//  * It cannot drop the distance map. dijkstra_shortest_paths builds its queue
//    as d_ary_heap_indirect<Vertex, 4, IndexInHeapMap, DistanceMap, ...>, so
//    the distance map *is* the heap's key. "streaming" here therefore means
//    dropping only the predecessor map.
//  * It has no early exit. Stopping the search means throwing from the visitor
//    and catching outside, which is the documented BGL idiom. The throw
//    happens once per query, so it is not on the hot path, but it is the only
//    way to express the k-nearest query at all.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

static constexpr std::size_t UNBOUNDED =
    std::numeric_limits<std::size_t>::max();

struct search_finished {};

// Consumes each settled vertex's distance, then aborts the search once k of
// them have been seen.
//
// Every piece of state is held by pointer, including the counter. Boost's
// named-parameter mechanism takes the visitor *by value*, so anything kept as
// a value member is incremented on a copy and lost -- which is exactly how the
// settled counter read zero for Boost while the timings and digests were
// correct.
template <typename _Value>
class bounded_visitor : public boost::default_dijkstra_visitor {
    const std::vector<_Value> * _distances;
    std::vector<_Value> * _settled;  // null when only the sum is wanted
    _Value * _total;
    std::size_t * _count;
    std::size_t _limit;

public:
    bounded_visitor(const std::vector<_Value> & distances,
                    std::vector<_Value> * settled, _Value & total,
                    std::size_t & count, std::size_t limit)
        : _distances(&distances)
        , _settled(settled)
        , _total(&total)
        , _count(&count)
        , _limit(limit) {}

    template <typename _Vertex, typename _Graph>
    void examine_vertex(_Vertex u, const _Graph &) {
        const _Value d = (*_distances)[static_cast<std::size_t>(u)];
        *_total += d;
        if(_settled != nullptr) _settled->push_back(d);
        if(++*_count >= _limit) throw search_finished{};
    }
};

template <typename _Graph, typename _Value, bool _Store, std::size_t _K,
          typename _Run>
void run_bounded(benchmark::State & state,
                 const std::filesystem::path & gr_file, _Graph & graph,
                 const std::vector<unsigned int> & sources, _Run && run) {
    const std::size_t nb_nodes = num_vertices(graph);

    state.SetLabel(cached_setup(gr_file, [&] {
        std::vector<_Value> settled;
        checksum cs;
        for(auto && s : sources) {
            settled.clear();
            std::vector<_Value> d(nb_nodes);
            _Value total = 0;
            std::size_t count = 0;
            bounded_visitor<_Value> vis(d, &settled, total, count, _K);
            try {
                run(graph, s, d, vis);
            } catch(const search_finished &) {
            }
            std::sort(settled.begin(), settled.end());
            cs.add(settled.size());
            for(const _Value & value : settled) cs.add(value);
        }
        return cs.str();
    }));

    std::size_t total_settled = 0;
    for(auto _ : state) {
        for(auto && s : sources) {
            std::vector<_Value> d(nb_nodes);
            _Value total = 0;
            std::size_t count = 0;
            bounded_visitor<_Value> vis(d, nullptr, total, count, _K);
            try {
                run(graph, s, d, vis);
            } catch(const search_finished &) {
            }
            benchmark::DoNotOptimize(total);
            total_settled += count;
        }
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    state.counters["settled"] =
        static_cast<double>(total_settled) /
        static_cast<double>(state.iterations() * sources.size());
}

template <typename _Value, bool _Store, std::size_t _K>
struct BM_csr {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_csr_dimacs<_Value>(gr_file); });
        using graph_t = std::decay_t<decltype(graph)>;
        using vertex_descriptor =
            typename graph_traits<graph_t>::vertex_descriptor;
        const std::size_t nb_nodes = num_vertices(graph);

        run_bounded<graph_t, _Value, _Store, _K>(
            state, gr_file, graph, sources,
            [nb_nodes](auto & g, auto s, auto & d, auto & vis) {
                auto weights = get(&Edge_Cost<_Value>::weight, g);
                if constexpr(_Store) {
                    std::vector<vertex_descriptor> p(nb_nodes);
                    dijkstra_shortest_paths(g, s,
                                            predecessor_map(&p[0])
                                                .distance_map(&d[0])
                                                .visitor(vis)
                                                .weight_map(weights));
                } else {
                    dijkstra_shortest_paths(
                        g, s,
                        distance_map(&d[0]).visitor(vis).weight_map(weights));
                }
            });
    }
};

#define REGISTER(value, store, mode, k, k_name)                                \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +         \
                                     "/compressed_sparse_row:" mode "/" #value \
                                     "/" k_name,                               \
                                 BM_csr<value, store, k>{}, gr_file, sources);

#define REGISTER_K(k, k_name)                \
    REGISTER(int, true, "stored", k, k_name) \
    REGISTER(int, false, "streaming", k, k_name)

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

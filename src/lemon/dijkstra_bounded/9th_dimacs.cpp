// Storage policy across query sizes -- LEMON side. See
// src/melon/dijkstra_bounded/9th_dimacs.cpp for the task definition.
//
// LEMON can express the streaming mode, and this gives it the leanest
// configuration its API allows: NullMap for the predecessor, distance and
// processed maps. That is legitimate because processNextNode() only ever
// *writes* those three -- the algorithm carries the current distance in the
// heap -- and currentDist(v) reads (*_heap)[v] for a vertex that has not been
// processed yet. So the loop takes the distance off the heap before popping
// and no O(|V|) map is written per settled vertex.
//
// Handicapping LEMON here would make MELON's advantage look bigger than it is.
// What LEMON cannot avoid is the heap cross-reference NodeMap<int>, which is
// allocated and initialized per query whatever the traits say.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/dijkstra.h>
#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <lemon/quad_heap.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

using namespace lemon;

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

static constexpr std::size_t UNBOUNDED =
    std::numeric_limits<std::size_t>::max();

template <typename _Graph, typename _Value, bool _Store>
struct DijkstraTraits {
    using Digraph = _Graph;
    using LengthMap = typename _Graph::template ArcMap<_Value>;
    using Value = _Value;
    using OperationTraits = DijkstraDefaultOperationTraits<Value>;
    using HeapCrossRef = typename Digraph::template NodeMap<int>;
    static HeapCrossRef * createHeapCrossRef(const Digraph & g) {
        return new HeapCrossRef(g);
    }

    using Heap = QuadHeap<_Value, HeapCrossRef, std::less<Value>>;
    static Heap * createHeap(HeapCrossRef & r) { return new Heap(r); }

    using PredMap = std::conditional_t<
        _Store, typename Digraph::template NodeMap<typename Digraph::Arc>,
        NullMap<typename Digraph::Node, typename Digraph::Arc>>;
    static PredMap * createPredMap(const Digraph & g) {
        if constexpr(_Store)
            return new PredMap(g);
        else
            return (void)g, new PredMap();
    }

    using ProcessedMap = NullMap<typename Digraph::Node, bool>;
    static ProcessedMap * createProcessedMap(const Digraph &) {
        return new ProcessedMap();
    }

    using DistMap =
        std::conditional_t<_Store, typename Digraph::template NodeMap<_Value>,
                           NullMap<typename Digraph::Node, _Value>>;
    static DistMap * createDistMap(const Digraph & g) {
        if constexpr(_Store)
            return new DistMap(g);
        else
            return (void)g, new DistMap();
    }
};

template <typename _Graph, typename _Value, bool _Store, std::size_t _K>
struct BM {
    using algorithm = Dijkstra<_Graph, typename _Graph::template ArcMap<_Value>,
                               DijkstraTraits<_Graph, _Value, _Store>>;

    static std::string result_checksum(
        const _Graph & graph,
        const typename _Graph::template ArcMap<_Value> & length_map,
        const std::vector<unsigned int> & sources) {
        std::vector<_Value> settled;
        checksum cs;
        for(auto && s : sources) {
            settled.clear();
            algorithm algo(graph, length_map);
            algo.init();
            algo.addSource(
                graph.fromId(static_cast<int>(s), typename _Graph::Node()));
            while(!algo.emptyQueue() && settled.size() < _K) {
                const auto v = algo.nextNode();
                settled.push_back(algo.currentDist(v));
                algo.processNextNode();
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
        _Graph graph;
        typename _Graph::template ArcMap<_Value> length_map(graph);
        parse_dimacs<_Graph, _Value>(gr_file, graph, length_map);

        state.SetLabel(result_checksum(graph, length_map, sources));

        std::size_t total_settled = 0;
        for(auto _ : state) {
            for(auto && s : sources) {
                algorithm algo(graph, length_map);
                algo.init();
                algo.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));
                std::size_t settled = 0;
                _Value total = 0;
                while(!algo.emptyQueue() && settled < _K) {
                    const auto v = algo.nextNode();
                    total += algo.currentDist(v);
                    algo.processNextNode();
                    ++settled;
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

#define REGISTER_K(k, k_name)                                   \
    REGISTER(StaticDigraph, int, true, "stored", k, k_name)     \
    REGISTER(StaticDigraph, int, false, "streaming", k, k_name) \
    REGISTER(SmartDigraph, int, true, "stored", k, k_name)      \
    REGISTER(SmartDigraph, int, false, "streaming", k, k_name)

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources = instance_sources(
            n, m, [](int n, int m) { return (n + m) * std::log(n); });
        REGISTER_K(100, "k100")
        REGISTER_K(1000, "k1000")
        REGISTER_K(10000, "k10000")
        REGISTER_K(UNBOUNDED, "kall")
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

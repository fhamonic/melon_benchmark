#include <cmath>
#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/dheap.h>
#include <lemon/dijkstra.h>
#include <lemon/list_graph.h>
#include <lemon/quad_heap.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

using namespace lemon;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph, typename _Value, int _Arity>
struct DijkstraTraits {
    using Digraph = _Graph;
    using LengthMap = typename _Graph::ArcMap<_Value>;
    using Value = _Value;
    using OperationTraits = DijkstraDefaultOperationTraits<Value>;
    using HeapCrossRef = typename Digraph::template NodeMap<int>;
    static HeapCrossRef * createHeapCrossRef(const Digraph & g) {
        return new HeapCrossRef(g);
    }

    using Heap = std::conditional_t<
        _Arity == 2, BinHeap<_Value, HeapCrossRef, std::less<Value>>,
        std::conditional_t<
            _Arity == 4, QuadHeap<_Value, HeapCrossRef, std::less<Value>>,
            DHeap<_Value, HeapCrossRef, _Arity, std::less<Value>>>>;

    // typedef DHeap<_Value, HeapCrossRef, _Arity, std::less<Value>> Heap;
    static Heap * createHeap(HeapCrossRef & r) { return new Heap(r); }

    typedef typename Digraph::template NodeMap<typename Digraph::Arc> PredMap;
    static PredMap * createPredMap(const Digraph & g) { return new PredMap(g); }

    typedef NullMap<typename Digraph::Node, bool> ProcessedMap;
    static ProcessedMap * createProcessedMap(const Digraph &) {
        return new ProcessedMap();
    }

    typedef typename Digraph::template NodeMap<_Value> DistMap;
    static DistMap * createDistMap(const Digraph & g) { return new DistMap(g); }
};

template <typename _Graph, typename _Value, int _Arity>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] =
            cached_parse_dimacs<_Graph, _Value>(gr_file);

        state.SetLabel(cached_setup(gr_file, [&] {
            checksum cs;
            for(auto && s : sources) {
                Dijkstra<_Graph, typename _Graph::ArcMap<_Value>,
                         DijkstraTraits<_Graph, _Value, _Arity>>
                    algo(graph, length_map);
                algo.init();
                algo.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));
                algo.start();
                lemon_add_distances(
                    graph, cs, [&](const auto & u) { return algo.reached(u); },
                    [&](const auto & u) { return algo.dist(u); });
            }
            return cs.str();
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                Dijkstra<_Graph, typename _Graph::ArcMap<_Value>,
                         DijkstraTraits<_Graph, _Value, _Arity>>
                    dijkstra(graph, length_map);

                dijkstra.init();
                dijkstra.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));
                while(!dijkstra.emptyQueue()) {
                    auto u = dijkstra.processNextNode();
                    benchmark::DoNotOptimize(dijkstra.dist(u));
                    // std::cout << dijkstra.dist(u) << std::endl;
                }
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER(graph, value, arity)                                          \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +         \
                                     "/" #graph "/" #value "/" #arity "-heap", \
                                 BM<graph, value, arity>{}, gr_file, sources);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources = instance_sources(
            n, m, [](int n, int m) { return (n + m) * std::log(n); });
        REGISTER(StaticDigraph, int, 2)
        REGISTER(StaticDigraph, int, 4)
        REGISTER(StaticDigraph, int, 8)
        REGISTER(StaticDigraph, double, 2)
        REGISTER(StaticDigraph, double, 4)
        REGISTER(StaticDigraph, double, 8)
        REGISTER(SmartDigraph, int, 2)
        REGISTER(SmartDigraph, int, 4)
        REGISTER(SmartDigraph, int, 8)
        REGISTER(SmartDigraph, double, 2)
        REGISTER(SmartDigraph, double, 4)
        REGISTER(SmartDigraph, double, 8)
        REGISTER(ListDigraph, int, 2)
        REGISTER(ListDigraph, int, 4)
        REGISTER(ListDigraph, int, 8)
        REGISTER(ListDigraph, double, 2)
        REGISTER(ListDigraph, double, 4)
        REGISTER(ListDigraph, double, 8)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

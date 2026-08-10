// Point-to-point shortest path -- LEMON side. See
// src/melon/dijkstra_point_to_point/9th_dimacs.cpp for the task.
//
// LEMON also offers Dijkstra::start(t), which stops when t reaches the top of
// the heap. The explicit loop is used instead so that the distance can be read
// off the heap via currentDist(v), which lets the predecessor, distance and
// processed maps all be NullMap -- the same leanest configuration the storage
// policy benchmark gives it, and the fair counterpart to MELON storing nothing.

#include <cmath>
#include <cstddef>
#include <filesystem>
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

template <typename _Graph, typename _Value>
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

    using PredMap = NullMap<typename Digraph::Node, typename Digraph::Arc>;
    static PredMap * createPredMap(const Digraph &) { return new PredMap(); }

    using ProcessedMap = NullMap<typename Digraph::Node, bool>;
    static ProcessedMap * createProcessedMap(const Digraph &) {
        return new ProcessedMap();
    }

    using DistMap = NullMap<typename Digraph::Node, _Value>;
    static DistMap * createDistMap(const Digraph &) { return new DistMap(); }
};

template <typename _Graph, typename _Value>
struct BM {
    using algorithm = Dijkstra<_Graph, typename _Graph::template ArcMap<_Value>,
                               DijkstraTraits<_Graph, _Value>>;

    template <typename _Run>
    static void for_each_query(
        const _Graph & graph,
        const typename _Graph::template ArcMap<_Value> & length_map,
        const std::vector<unsigned int> & sources, int num_vertices,
        _Run && consume) {
        for(auto && s : sources) {
            const auto target = graph.nodeFromId(
                static_cast<int>(instance_target(s, num_vertices)));
            algorithm algo(graph, length_map);
            algo.init();
            algo.addSource(
                graph.fromId(static_cast<int>(s), typename _Graph::Node()));
            bool found = false;
            while(!algo.emptyQueue()) {
                const auto v = algo.nextNode();
                const auto dv = algo.currentDist(v);
                algo.processNextNode();
                if(v == target) {
                    consume(true, dv);
                    found = true;
                    break;
                }
            }
            if(!found) consume(false, _Value{});
        }
    }

    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources,
                    int num_vertices) const {
        _Graph graph;
        typename _Graph::template ArcMap<_Value> length_map(graph);
        parse_dimacs<_Graph, _Value>(gr_file, graph, length_map);

        {
            checksum cs;
            for_each_query(graph, length_map, sources, num_vertices,
                           [&](bool found, _Value d) {
                               if(found)
                                   cs.add(d);
                               else
                                   cs.add_unreached();
                           });
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            _Value acc = 0;
            for_each_query(graph, length_map, sources, num_vertices,
                           [&](bool found, _Value d) {
                               if(found) acc += d;
                           });
            benchmark::DoNotOptimize(acc);
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER(graph, value)                                             \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +     \
                                     "/" #graph ":unidirectional/" #value, \
                                 BM<graph, value>{}, gr_file, sources, n);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources = instance_sources(
            n, m, [](int n, int m) { return (n + m) * std::log(n); });
        REGISTER(StaticDigraph, int)
        REGISTER(SmartDigraph, int)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

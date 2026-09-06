#include <cmath>
#include <filesystem>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

// Consumes the distance of each vertex as it is settled, which is what MELON's
// and LEMON's benchmark loops do when they iterate the traversal. The previous
// version instead swept all |V| distances *after* the run, inside the timed
// region, charging Boost for an O(n) pass its competitors never paid.
template <typename _Value>
class consume_distance_visitor : public boost::default_dijkstra_visitor {
    const std::vector<_Value> * _distances;

public:
    explicit consume_distance_visitor(const std::vector<_Value> & d)
        : _distances(&d) {}

    template <typename _Vertex, typename _Graph>
    void examine_vertex(_Vertex u, const _Graph &) {
        benchmark::DoNotOptimize((*_distances)[static_cast<std::size_t>(u)]);
    }
};

// Storage policy matches the other two libraries: a distance map and a
// predecessor map, both allocated per query.
//
// The distance map is `_Value`, not `int`. It used to be `std::vector<int>`
// for both value types, so Boost deduced `closed_plus<int>` and the "double"
// series silently accumulated truncated integer arithmetic -- it was not
// measuring a double-precision Dijkstra at all.
template <typename _Graph, typename _Value, typename _Run>
void run_dijkstra(benchmark::State & state,
                  const std::filesystem::path & gr_file, _Graph & graph,
                  const std::vector<unsigned int> & sources, _Run && run) {
    const std::size_t nb_nodes = num_vertices(graph);
    using vertex_descriptor = typename graph_traits<_Graph>::vertex_descriptor;

    state.SetLabel(cached_setup(gr_file, [&] {
        checksum cs;
        for(auto && s : sources) {
            std::vector<vertex_descriptor> p(nb_nodes);
            std::vector<_Value> d(nb_nodes);
            run(graph, s, p, d, boost::default_dijkstra_visitor{});
            boost_add_distances(cs, d);
        }
        return cs.str();
    }));

    for(auto _ : state) {
        for(auto && s : sources) {
            std::vector<vertex_descriptor> p(nb_nodes);
            std::vector<_Value> d(nb_nodes);
            run(graph, s, p, d, consume_distance_visitor<_Value>(d));
        }
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
}

template <typename _Value>
struct BM_adj_dijkstra {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_adj_list_dimacs<_Value>(gr_file); });
        run_dijkstra<decltype(graph), _Value>(
            state, gr_file, graph, sources,
            [](auto & g, auto s, auto & p, auto & d, auto vis) {
                dijkstra_shortest_paths(
                    g, s,
                    predecessor_map(&p[0]).distance_map(&d[0]).visitor(vis));
            });
    }
};

#define REGISTER_ADJ_LIST(value)                                          \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +    \
                                     "/adjacency_list/" #value "/4-heap", \
                                 BM_adj_dijkstra<value>{}, gr_file, sources);

template <typename _Value>
struct BM_csr_dijkstra {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_csr_dimacs<_Value>(gr_file); });
        run_dijkstra<decltype(graph), _Value>(
            state, gr_file, graph, sources,
            [](auto & g, auto s, auto & p, auto & d, auto vis) {
                dijkstra_shortest_paths(
                    g, s,
                    predecessor_map(&p[0])
                        .distance_map(&d[0])
                        .visitor(vis)
                        .weight_map(get(&Edge_Cost<_Value>::weight, g)));
            });
    }
};

#define REGISTER_CSR(value)                                            \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) + \
                                     "/compressed_sparse_row/" #value  \
                                     "/4-heap",                        \
                                 BM_csr_dijkstra<value>{}, gr_file, sources);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, num_vertices, num_arcs] : instances) {
        const auto sources = instance_sources(
            num_vertices, num_arcs,
            [](int n, int m) { return (n + m) * std::log(n); });
        // Boost's dijkstra_shortest_paths defaults to d_ary_heap_indirect with
        // arity 4 (boost/graph/dijkstra_shortest_paths.hpp). These runs used to
        // be registered as "2-heap", which landed them in the chart comparing
        // MELON's and LEMON's *binary* heaps.
        REGISTER_ADJ_LIST(int)
        REGISTER_ADJ_LIST(double)
        REGISTER_CSR(int)
        REGISTER_CSR(double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

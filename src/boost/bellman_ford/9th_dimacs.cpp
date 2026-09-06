#include <cmath>
#include <filesystem>
#include <limits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

// Storage policy matches the other two libraries: a distance map and a
// predecessor map, both allocated per query. The maps need no initialization
// here: passing root_vertex makes bellman_ford_shortest_paths fill them
// itself (distances to the type's max, the source to zero) -- an init loop
// added here would be timed twice.
//
// Boost's final edge sweep certifying the absence of a negative cycle cannot
// be turned off; MELON and LEMON run with detection enabled so nobody skips
// that work.
template <typename _Graph, typename _Value, typename _Run>
void run_bellman_ford(benchmark::State & state,
                      const std::filesystem::path & gr_file, _Graph & graph,
                      const std::vector<unsigned int> & sources, _Run && run) {
    const std::size_t nb_nodes = num_vertices(graph);
    using vertex_descriptor = typename graph_traits<_Graph>::vertex_descriptor;

    state.SetLabel(cached_setup(gr_file, [&] {
        checksum cs;
        for(auto && s : sources) {
            std::vector<vertex_descriptor> p(nb_nodes);
            std::vector<_Value> d(nb_nodes);
            run(graph, s, p, d);
            boost_add_distances(cs, d);
        }
        return cs.str();
    }));

    for(auto _ : state) {
        for(auto && s : sources) {
            std::vector<vertex_descriptor> p(nb_nodes);
            std::vector<_Value> d(nb_nodes);
            run(graph, s, p, d);
            // The O(n) read-back stays inside the timed region because the
            // MELON and LEMON loops pay the same sweep.
            for(const _Value & dist : d) {
                benchmark::DoNotOptimize(dist);
            }
        }
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
}

template <typename _Value>
struct BM_adj_bellman_ford {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_adj_list_dimacs<_Value>(gr_file); });
        run_bellman_ford<decltype(graph), _Value>(
            state, gr_file, graph, sources,
            [](auto & g, auto s, auto & p, auto & d) {
                benchmark::DoNotOptimize(bellman_ford_shortest_paths(
                    g, num_vertices(g),
                    predecessor_map(&p[0]).distance_map(&d[0]).root_vertex(s)));
            });
    }
};

#define REGISTER_ADJ_LIST(value)                                         \
    benchmark::RegisterBenchmark(                                        \
        std::string(gr_file.stem().c_str()) + "/adjacency_list/" #value, \
        BM_adj_bellman_ford<value>{}, gr_file, sources);

template <typename _Value>
struct BM_csr_bellman_ford {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_csr_dimacs<_Value>(gr_file); });
        run_bellman_ford<decltype(graph), _Value>(
            state, gr_file, graph, sources,
            [](auto & g, auto s, auto & p, auto & d) {
                benchmark::DoNotOptimize(bellman_ford_shortest_paths(
                    g, num_vertices(g),
                    predecessor_map(&p[0])
                        .distance_map(&d[0])
                        .root_vertex(s)
                        .weight_map(get(&Edge_Cost<_Value>::weight, g))));
            });
    }
};

#define REGISTER_CSR(value)                                             \
    benchmark::RegisterBenchmark(std::string(gr_file.stem().c_str()) +  \
                                     "/compressed_sparse_row/" #value,  \
                                 BM_csr_bellman_ford<value>{}, gr_file, \
                                 sources);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    // Bellman-Ford's cost model is n*m, not Dijkstra's (n+m)*log(n). Getting
    // that wrong understated the work by five orders of magnitude on the
    // larger road networks, where one run takes tens of seconds; it is also
    // what decides whether an instance is worth running at all.
    const auto complexity = [](int n, int m) {
        return static_cast<double>(n) * static_cast<double>(m);
    };
    for(const auto & [gr_file, num_vertices, num_arcs] : instances) {
        if(!instance_is_affordable(complexity(num_vertices, num_arcs)))
            continue;
        const auto sources =
            instance_sources(num_vertices, num_arcs, complexity);
        REGISTER_ADJ_LIST(int)
        REGISTER_ADJ_LIST(double)
        REGISTER_CSR(int)
        REGISTER_CSR(double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

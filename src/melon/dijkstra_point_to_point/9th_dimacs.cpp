// Point-to-point shortest path: the distance from a source to one target.
//
// This is the only shortest-path question CXXGraph's API can answer, so it is
// the benchmark where all four libraries appear. MELON, LEMON and Boost stop
// as soon as the target is settled; CXXGraph cannot, and pays a full
// single-source computation per query. See
// src/cxxgraph/dijkstra_point_to_point/ for that.
//
// MELON is registered twice, as "<container>:unidirectional" and
// "<container>:bidirectional". The unidirectional bars are the same-algorithm
// comparison against LEMON, Boost and CXXGraph. The bidirectional one is a
// different algorithm -- it grows a search from each end and stops when the
// two meet -- and MELON is the only one of the four that ships it, so that bar
// answers "what does this library give you for this query" rather than "whose
// Dijkstra is faster". Both must produce the same distances, and the digest
// enforces it.
//
// The unidirectional variant stores nothing -- the caller wants one number, so
// both storage traits are off and the distance is taken as the target is
// settled. The bidirectional one is forced to store its paths; see the note on
// bench_bidirectional_traits below.

#include <cmath>
#include <filesystem>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

#include "melon/algorithm/bidirectional_dijkstra.hpp"
#include "melon/algorithm/dijkstra.hpp"

using namespace melon;

template <typename _Graph, typename _Value>
struct bench_dijkstra_traits {
    using semiring = shortest_path_semiring<_Value>;
    using heap =
        updatable_d_ary_heap<4, std::pair<vertex_t<_Graph>, _Value>,
                             std::less<_Value>,
                             vertex_map_t<_Graph, std::size_t>,
                             maps::element<1>, maps::element<0>>;

    static constexpr bool store_distances = false;
    static constexpr bool store_paths = false;
};

template <typename _Graph, typename _Value>
struct bench_bidirectional_traits {
    using semiring = shortest_path_semiring<_Value>;
    using heap =
        updatable_d_ary_heap<4, std::pair<vertex_t<_Graph>, _Value>,
                             typename semiring::less_t,
                             vertex_map_t<_Graph, std::size_t>,
                             maps::element<1>, maps::element<0>>;

    // Only the distance is wanted, so the path is not reconstructed -- the
    // same policy as the unidirectional variant it is compared against.
    static constexpr bool store_paths = false;
};

template <typename _Graph, typename _Value>
struct BM {
    using traits = bench_dijkstra_traits<_Graph, _Value>;

    // Distance to the target, or unreached, for every source.
    static std::string result_checksum(
        const _Graph & graph, const auto & length_map,
        const std::vector<unsigned int> & sources, int num_vertices) {
        checksum cs;
        for(auto && s : sources) {
            const auto target =
                static_cast<vertex_t<_Graph>>(instance_target(s, num_vertices));
            bool found = false;
            for(auto && [u, dist] : dijkstra(traits{}, graph, length_map, s)) {
                if(u == target) {
                    cs.add(dist);
                    found = true;
                    break;
                }
            }
            if(!found) cs.add_unreached();
        }
        return cs.str();
    }

    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources,
                    int num_vertices) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_dimacs<_Graph, _Value>(gr_file); });

        state.SetLabel(cached_setup(gr_file, [&] {
            return result_checksum(graph, length_map, sources, num_vertices);
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                const auto target = static_cast<vertex_t<_Graph>>(
                    instance_target(s, num_vertices));
                _Value found = 0;
                for(auto && [u, dist] :
                    dijkstra(traits{}, graph, length_map, s)) {
                    if(u == target) {
                        found = dist;
                        break;
                    }
                }
                benchmark::DoNotOptimize(found);
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

template <typename _Graph, typename _Value>
struct BM_bidirectional {
    using traits = bench_bidirectional_traits<_Graph, _Value>;
    static constexpr _Value unreached = traits::semiring::infty;

    static std::string result_checksum(
        const _Graph & graph, const auto & length_map,
        const std::vector<unsigned int> & sources, int num_vertices) {
        checksum cs;
        for(auto && s : sources) {
            const auto target =
                static_cast<vertex_t<_Graph>>(instance_target(s, num_vertices));
            auto algo =
                bidirectional_dijkstra(traits{}, graph, length_map, s, target);
            // run() returns *this for chaining; the s-t distance is dist().
            const auto distance = algo.run().dist();
            if(distance == unreached)
                cs.add_unreached();
            else
                cs.add(distance);
        }
        return cs.str();
    }

    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources,
                    int num_vertices) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_dimacs<_Graph, _Value>(gr_file); });

        state.SetLabel(cached_setup(gr_file, [&] {
            return result_checksum(graph, length_map, sources, num_vertices);
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                const auto target = static_cast<vertex_t<_Graph>>(
                    instance_target(s, num_vertices));
                auto algo = bidirectional_dijkstra(traits{}, graph, length_map,
                                                   s, target);
                benchmark::DoNotOptimize(algo.run().dist());
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER(bm, graph, mode, value)                                      \
    benchmark::RegisterBenchmark(                                             \
        std::string(gr_file.stem().c_str()) + "/" #graph ":" mode "/" #value, \
        bm<graph, value>{}, gr_file, sources, n);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources = instance_sources(
            n, m, [](int n, int m) { return (n + m) * std::log(n); });
        REGISTER(BM, static_digraph, "unidirectional", int)
        REGISTER(BM, mutable_digraph, "unidirectional", int)
        REGISTER(BM_bidirectional, static_digraph, "bidirectional", int)
        REGISTER(BM_bidirectional, mutable_digraph, "bidirectional", int)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

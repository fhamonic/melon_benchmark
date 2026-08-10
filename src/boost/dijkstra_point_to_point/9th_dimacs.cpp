// Point-to-point shortest path -- Boost side. See
// src/melon/dijkstra_point_to_point/9th_dimacs.cpp for the task.
//
// Boost has no early exit, so stopping at the target means throwing from the
// visitor and catching outside; that is the documented BGL idiom and it costs
// one throw per query, off the hot path. It still cannot drop the distance map
// -- that map is the heap's key -- so unlike MELON and LEMON it allocates
// O(|V|) per query even though the caller wants a single number.

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

struct target_reached {};

// State is held by pointer: BGL takes the visitor by value.
class stop_at_target_visitor : public boost::default_dijkstra_visitor {
    std::size_t _target;

public:
    explicit stop_at_target_visitor(std::size_t target) : _target(target) {}

    template <typename _Vertex, typename _Graph>
    void examine_vertex(_Vertex u, const _Graph &) {
        if(static_cast<std::size_t>(u) == _target) throw target_reached{};
    }
};

template <typename _Value>
struct BM_csr {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources,
                    int num_vertices) const {
        auto [graph, length_map] = parse_csr_dimacs<_Value>(gr_file);
        const std::size_t nb_nodes = num_vertices_of(graph);

        const auto query = [&](unsigned int s, std::vector<_Value> & d) {
            const std::size_t target = instance_target(s, num_vertices);
            try {
                dijkstra_shortest_paths(
                    graph, s,
                    distance_map(&d[0])
                        .visitor(stop_at_target_visitor(target))
                        .weight_map(get(&Edge_Cost<_Value>::weight, graph)));
            } catch(const target_reached &) {
            }
            return d[target];
        };

        {
            checksum cs;
            for(auto && s : sources) {
                std::vector<_Value> d(nb_nodes);
                const _Value distance = query(s, d);
                if(distance == (std::numeric_limits<_Value>::max)())
                    cs.add_unreached();
                else
                    cs.add(distance);
            }
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            for(auto && s : sources) {
                std::vector<_Value> d(nb_nodes);
                benchmark::DoNotOptimize(query(s, d));
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }

private:
    template <typename _Graph>
    static std::size_t num_vertices_of(const _Graph & g) {
        return num_vertices(g);
    }
};

#define REGISTER(value)                                      \
    benchmark::RegisterBenchmark(                            \
        std::string(gr_file.stem().c_str()) +                \
            "/compressed_sparse_row:unidirectional/" #value, \
        BM_csr<value>{}, gr_file, sources, n);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources = instance_sources(
            n, m, [](int n, int m) { return (n + m) * std::log(n); });
        REGISTER(int)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

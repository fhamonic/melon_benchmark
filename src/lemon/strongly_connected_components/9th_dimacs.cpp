#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/connectivity.h>
#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

using namespace lemon;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        _Graph graph;
        typename _Graph::ArcMap<int> length_map(graph);

        parse_dimacs<_Graph, int>(gr_file, graph, length_map);

        {
            typename _Graph::NodeMap<int> compMap(graph);
            lemon::stronglyConnectedComponents(graph, compMap);
            state.SetLabel(lemon_partition_checksum(graph, compMap));
        }

        for(auto _ : state) {
            typename _Graph::NodeMap<int> compMap(graph);
            lemon::stronglyConnectedComponents(graph, compMap);
            benchmark::DoNotOptimize(compMap);
        }
    }
};

#define REGISTER(graph)                                                \
    benchmark::RegisterBenchmark(                                      \
        std::string(gr_file.stem().c_str()) + "/" #graph, BM<graph>{}, \
        gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(StaticDigraph)
        REGISTER(SmartDigraph)
        REGISTER(ListDigraph)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

#include <filesystem>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/algorithm/connected_components.hpp"
#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"

using namespace melon;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        auto [graph, lengths] = parse_dimacs<_Graph, int>(gr_file);

        state.SetLabel(melon_partition_checksum(
            graph, weakly_connected_components(graph)));

        for(auto _ : state) {
            for(auto && component : weakly_connected_components(graph)) {
                for(auto v : component) {
                    benchmark::DoNotOptimize(v);
                }
            }
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
        REGISTER(static_digraph)
        REGISTER(mutable_digraph)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

#include <filesystem>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/algorithm/breadth_first_search.hpp"
#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"

using namespace melon;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto [graph, length_map] = parse_dimacs<_Graph, int>(gr_file);

        state.SetLabel(melon_traversal_checksum(graph, sources, [&](auto && s) {
            return breadth_first_search(graph, s);
        }));
        for(auto _ : state) {
            for(auto && s : sources) {
                for(auto v : breadth_first_search(graph, s)) {
                    benchmark::DoNotOptimize(v);
                }
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER(graph)                                                \
    benchmark::RegisterBenchmark(                                      \
        std::string(gr_file.stem().c_str()) + "/" #graph, BM<graph>{}, \
        gr_file, sources);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources =
            instance_sources(n, m, [](int n, int m) { return n + m; });
        REGISTER(static_digraph)
        REGISTER(mutable_digraph)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

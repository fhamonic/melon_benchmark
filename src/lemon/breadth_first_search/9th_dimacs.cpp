#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/bfs.h>
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
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse_dimacs<_Graph, int>(gr_file);

        state.SetLabel(cached_setup(gr_file, [&] {
            checksum cs;
            for(auto && s : sources) {
                Bfs<_Graph> algo(graph);
                algo.init();
                algo.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));
                algo.start();
                lemon_add_reachability(
                    graph, cs, [&](const auto & u) { return algo.reached(u); });
            }
            return cs.str();
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                Bfs<_Graph> bfs(graph);
                bfs.init();
                bfs.addSource(
                    graph.fromId(static_cast<int>(s), typename _Graph::Node()));

                while(!bfs.emptyQueue()) {
                    benchmark::DoNotOptimize(bfs.processNextNode());
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
        REGISTER(StaticDigraph)
        REGISTER(SmartDigraph)
        REGISTER(ListDigraph)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

// CXXGraph DFS.
//
// Registered on rome99 only, for two reasons. CXXGraph's depth_first_search
// keeps its visited set in a std::vector and does a linear std::find over it
// for every edge it examines, so the traversal is O(|V|.|A|) rather than
// O(|V|+|A|) -- the same shape as its BFS, which needs ~38 s for one search on
// USA-road-d.NY against MELON's ~16 ms. And it recurses once per settled
// vertex, so on any of the USA networks the stack depth would be in the
// hundreds of thousands.
//
// rome99 is small enough on both counts, and every other library runs it too,
// so the chart carries a real four-way comparison on one instance rather than
// no CXXGraph bar at all.

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "CXXGraph/CXXGraph.hpp"

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto instance = parse_dimacs(gr_file);
        const std::size_t nb_nodes = instance.nodes.size();

        {
            checksum cs;
            for(auto && s : sources)
                cxxgraph_add_reachability(
                    cs, nb_nodes,
                    instance.graph.depth_first_search(*instance.nodes[s]));
            state.SetLabel(cs.str());
        }

        std::size_t total_visited = 0;
        for(auto _ : state) {
            for(auto && s : sources) {
                auto visited =
                    instance.graph.depth_first_search(*instance.nodes[s]);
                benchmark::DoNotOptimize(visited.size());
                total_visited += visited.size();
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
        state.counters["visited"] =
            static_cast<double>(total_visited) /
            static_cast<double>(state.iterations() * sources.size());
    }
};

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        if(gr_file.stem() != "rome99") continue;  // see the note at the top
        const auto sources =
            instance_sources(n, m, [](int n, int m) { return n + m; });
        benchmark::RegisterBenchmark(
            std::string(gr_file.stem().c_str()) + "/shared_ptr_edge_set", BM{},
            gr_file, sources);
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

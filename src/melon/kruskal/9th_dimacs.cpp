#include <filesystem>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/algorithm/kruskal.hpp"
#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"
#include "melon/views/undirect.hpp"

using namespace melon;

#include "checksum.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

template <typename _Graph, typename _Value>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        auto [graph, costs] = parse_dimacs<_Graph, _Value>(gr_file);

        {
            std::size_t num_edges = 0;
            _Value total = 0;
            for(auto && e : kruskal(views::undirect(graph), costs)) {
                ++num_edges;
                total += costs[e];
            }
            checksum cs;
            cs.add(num_edges);
            cs.add(total);
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            for(auto && e : kruskal(views::undirect(graph), costs)) {
                benchmark::DoNotOptimize(e);
            }
        }
    }
};

#define REGISTER(graph, graph_name, value)                               \
    benchmark::RegisterBenchmark(                                        \
        std::string(gr_file.stem().c_str()) + "/" graph_name "/" #value, \
        BM<graph, value>{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER(static_digraph, "undirect<static_digraph>", int)
        REGISTER(mutable_digraph, "undirect<mutable_digraph>", int)
        REGISTER(static_digraph, "undirect<static_digraph>", double)
        REGISTER(mutable_digraph, "undirect<mutable_digraph>", double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

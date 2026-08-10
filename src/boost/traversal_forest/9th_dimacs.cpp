#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/strong_components.hpp>

using namespace boost;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

struct BM_csr_strongly_sonnected_components {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        auto [graph, length_map] = parse_csr_dimacs<int>(gr_file);
        using graph_t = std::decay_t<decltype(graph)>;
        const int nb_nodes = num_vertices(graph);

        {
            std::vector<int> compMap(static_cast<std::size_t>(nb_nodes));
            boost::connected_components(
                graph,
                make_iterator_property_map(
                    compMap.begin(), boost::get(boost::vertex_index, graph)));
            state.SetLabel(boost_partition_checksum(compMap));
        }

        for(auto _ : state) {
            std::vector<int> compMap(nb_nodes);
            int nb_components = boost::connected_components(
                graph,
                make_iterator_property_map(
                    compMap.begin(), boost::get(boost::vertex_index, graph)));
            benchmark::DoNotOptimize(nb_components);
        }
    }
};

#define REGISTER_CSR()                                                  \
    benchmark::RegisterBenchmark(                                       \
        std::string(gr_file.stem().c_str()) + "/compressed_sparse_row", \
        BM_csr_strongly_sonnected_components{}, gr_file);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        REGISTER_CSR()
        // REGISTER_ADJ_LIST()
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

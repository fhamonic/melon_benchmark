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
#include "parse_snap.hpp"
#include "snap_instances.hpp"

struct BM_csr_strongly_connected_components {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file) const {
        auto graph = parse_csr_snap(gr_file);
        const int nb_nodes = num_vertices(graph);

        {
            std::vector<int> compMap(static_cast<std::size_t>(nb_nodes));
            boost::strong_components(
                graph,
                make_iterator_property_map(
                    compMap.begin(), boost::get(boost::vertex_index, graph)));
            state.SetLabel(boost_partition_checksum(compMap));
        }

        for(auto _ : state) {
            std::vector<int> compMap(nb_nodes);
            int nb_components = boost::strong_components(
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
        BM_csr_strongly_connected_components{}, gr_file);

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

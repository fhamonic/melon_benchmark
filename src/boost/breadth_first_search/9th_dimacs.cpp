#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "checksums.hpp"
#include "dimacs_instances.hpp"
#include "parse_dimacs.hpp"

class my_visitor : public boost::default_bfs_visitor {
public:
    my_visitor() {}
    void discover_vertex(const auto & v, const auto & g) {
        benchmark::DoNotOptimize(static_cast<std::size_t>(v));
    }
};

// Records which vertices the traversal discovers, for the result digest.
class reachability_visitor : public boost::default_bfs_visitor {
    std::vector<char> * _reached;

public:
    explicit reachability_visitor(std::vector<char> & reached)
        : _reached(&reached) {}
    void discover_vertex(const auto & v, const auto & g) {
        (*_reached)[static_cast<std::size_t>(v)] = 1;
    }
};

struct BM_adj_breadth_first_search {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_adj_list_dimacs<int>(gr_file); });

        state.SetLabel(cached_setup(gr_file, [&] {
            checksum cs;
            for(auto && s : sources) {
                std::vector<char> reached(num_vertices(graph), 0);
                reachability_visitor vis(reached);
                boost::breadth_first_search(graph, s, boost::visitor(vis));
                boost_add_reachability(cs, reached);
            }
            return cs.str();
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                my_visitor vis;
                boost::breadth_first_search(graph, s, boost::visitor(vis));
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER_ADJ_LIST(value)                                 \
    benchmark::RegisterBenchmark(                                \
        std::string(gr_file.stem().c_str()) + "/adjacency_list", \
        BM_adj_breadth_first_search{}, gr_file, sources);

struct BM_csr_breadth_first_search {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto & [graph, length_map] = cached_parse(
            gr_file, [&] { return parse_csr_dimacs<int>(gr_file); });

        state.SetLabel(cached_setup(gr_file, [&] {
            checksum cs;
            for(auto && s : sources) {
                std::vector<char> reached(num_vertices(graph), 0);
                reachability_visitor vis(reached);
                boost::breadth_first_search(graph, s, boost::visitor(vis));
                boost_add_reachability(cs, reached);
            }
            return cs.str();
        }));

        for(auto _ : state) {
            for(auto && s : sources) {
                my_visitor vis;
                boost::breadth_first_search(graph, s, boost::visitor(vis));
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER_CSR()                                                  \
    benchmark::RegisterBenchmark(                                       \
        std::string(gr_file.stem().c_str()) + "/compressed_sparse_row", \
        BM_csr_breadth_first_search{}, gr_file, sources);

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [gr_file, n, m] : instances) {
        const auto sources =
            instance_sources(n, m, [](int n, int m) { return n + m; });
        REGISTER_CSR()
        REGISTER_ADJ_LIST()
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

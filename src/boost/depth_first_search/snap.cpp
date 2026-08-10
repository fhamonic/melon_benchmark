#include <filesystem>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <algorithm>

#include <benchmark/benchmark.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>

using namespace boost;

#include "checksums.hpp"
#include "parse_snap.hpp"
#include "snap_instances.hpp"

class my_visitor : public boost::default_dfs_visitor {
public:
    my_visitor() {}
    void discover_vertex(const auto & v, const auto & g) {
        benchmark::DoNotOptimize(static_cast<std::size_t>(v));
    }
};

// Records which vertices the traversal discovers, for the result digest.
class reachability_visitor : public boost::default_dfs_visitor {
    std::vector<char> * _reached;

public:
    explicit reachability_visitor(std::vector<char> & reached)
        : _reached(&reached) {}
    void discover_vertex(const auto & v, const auto & g) {
        (*_reached)[static_cast<std::size_t>(v)] = 1;
    }
};

// boost::depth_first_search sweeps the *whole* graph: it starts at root_vertex
// and then keeps going with every remaining unvisited vertex, producing a DFS
// forest. MELON's depth_first_search(graph, s) and LEMON's Dfs from a single
// source explore only what is reachable from s. Boost was therefore doing
// |V|+|A| work per source where the others did the work of one component --
// the result digests disagreed, which is how this surfaced.
//
// depth_first_visit is the actual counterpart: one root, reachable set only.
// The colour map is its per-query state, allocated per source like the
// working state of the other two libraries.
template <typename _Graph, typename _Visitor>
void depth_first_visit_from(const _Graph & graph, unsigned int source,
                            _Visitor & vis,
                            std::vector<boost::default_color_type> & colors) {
    std::fill(colors.begin(), colors.end(), boost::white_color);
    boost::depth_first_visit(
        graph, vertex(source, graph), vis,
        make_iterator_property_map(colors.begin(),
                                   boost::get(boost::vertex_index, graph)));
}

struct BM_adj_depth_first_search {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto graph = parse_adj_list_snap(gr_file);

        {
            checksum cs;
            for(auto && s : sources) {
                std::vector<char> reached(num_vertices(graph), 0);
                std::vector<boost::default_color_type> colors(
                    num_vertices(graph));
                reachability_visitor vis(reached);
                depth_first_visit_from(graph, s, vis, colors);
                boost_add_reachability(cs, reached);
            }
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            for(auto && s : sources) {
                my_visitor vis;
                std::vector<boost::default_color_type> colors(
                    num_vertices(graph));
                depth_first_visit_from(graph, s, vis, colors);
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER_ADJ_LIST(value)                                 \
    benchmark::RegisterBenchmark(                                \
        std::string(gr_file.stem().c_str()) + "/adjacency_list", \
        BM_adj_depth_first_search{}, gr_file, sources);

struct BM_csr_depth_first_search {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & gr_file,
                    const std::vector<unsigned int> & sources) const {
        auto graph = parse_csr_snap(gr_file);

        {
            checksum cs;
            for(auto && s : sources) {
                std::vector<char> reached(num_vertices(graph), 0);
                std::vector<boost::default_color_type> colors(
                    num_vertices(graph));
                reachability_visitor vis(reached);
                depth_first_visit_from(graph, s, vis, colors);
                boost_add_reachability(cs, reached);
            }
            state.SetLabel(cs.str());
        }

        for(auto _ : state) {
            for(auto && s : sources) {
                my_visitor vis;
                std::vector<boost::default_color_type> colors(
                    num_vertices(graph));
                depth_first_visit_from(graph, s, vis, colors);
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * sources.size());
    }
};

#define REGISTER_CSR()                                                  \
    benchmark::RegisterBenchmark(                                       \
        std::string(gr_file.stem().c_str()) + "/compressed_sparse_row", \
        BM_csr_depth_first_search{}, gr_file, sources);

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

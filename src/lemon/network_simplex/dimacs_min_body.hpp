#pragma once

// Dataset-independent body: the including .cpp provides the
// `instances` list (a <dataset>_instances.hpp include) before
// including this header.
//
// LEMON's network simplex, the baseline MELON's is measured against.
//
// The registered names carry "<container>:network_simplex" so both libraries
// land in the same chart, as they do for max flow.
//
// Matched parameters. Both sides run the block-search pivot rule -- passed
// explicitly here rather than left to LEMON's default -- with a block of
// max(1.0 * sqrt(searched arcs), 10): LEMON's BLOCK_SIZE_FACTOR and
// MIN_BLOCK_SIZE against MELON's network_simplex_default_traits, which carry
// the same two constants. The searched arc counts coincide too: every
// generated instance has supplies summing to zero, so LEMON takes its EQ
// branch, where _search_arc_num is the arc count and the artificial arcs stay
// outside the search -- the same set MELON scans, which excludes its virtual
// arcs by construction.
//
// Arc mixing is swept rather than fixed, because it is the single biggest
// difference between the two implementations and which way it cuts depends on
// the instance family. LEMON copies the arcs into its own arrays in a
// scattered order (stride max(m/n, 3)), so a block-search block holds arcs
// from many different source vertices; unmixed it scans the order the graph
// yields, and both of MELON's containers group arcs by source -- 87.5% of
// consecutive arcs share a source on these instances. Decorrelated blocks
// make better entering-arc choices, correlated ones make the reduced-cost
// gather cheaper, and no single setting is the honest comparison:
//
//   .../mixed    LEMON's default -- what the library actually gives you.
//   .../unmixed  the arcs in the order the graph stores them.
//
// MELON registers the same two series (its own mixing is the same
// permutation applied as a scan order, and is off by default), so each side's
// setting is charted against its counterpart rather than against a mixture.
//
// One asymmetry is left, because LEMON has no switch for it: run() always
// begins with initialPivots(), a greedy warm start (one min-cost arc per
// supply and demand node) that MELON has no counterpart to. Disabling it in a
// patched LEMON costs about 4% of its pivots, so it is worth naming but does
// not carry the difference between the two.

#include <filesystem>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <lemon/list_graph.h>
#include <lemon/smart_graph.h>
#include <lemon/static_graph.h>

#include <lemon/network_simplex.h>

using namespace lemon;

#include "checksum.hpp"
#include "parse_dimacs_min.hpp"
#include "reference_solution.hpp"

template <typename _Graph, typename _Value, bool _Mixing>
struct BM {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & min_file) const {
        using arc_map = typename _Graph::template ArcMap<_Value>;
        using node_map = typename _Graph::template NodeMap<_Value>;
        using algorithm = NetworkSimplex<_Graph, _Value, _Value>;
        // Costs are accumulated in a type wide enough for the sum over every
        // arc, as MELON's total_cost() does; the `int` series would otherwise
        // overflow its own Cost type on the larger instances.
        using total_cost =
            std::conditional_t<std::is_integral_v<_Value>, long long, _Value>;

        auto & [graph, capacities, costs, supplies] =
            cached_parse_dimacs_min<_Graph, _Value>(min_file);

        // Total supply equals total demand in every generated instance, which
        // makes LEMON's default GEQ constraints equalities -- the form MELON
        // solves.
        const auto configure = [&](algorithm & algo) -> algorithm & {
            return algo.upperMap(capacities).costMap(costs).supplyMap(supplies);
        };

        // The reference check is a property of the instance, not of the
        // repetition, so it is cached together with the digest it guards.
        struct verified {
            std::string label;
            std::string error;
        };
        const auto & result = cached_setup(min_file, [&] {
            algorithm algo(graph, _Mixing);
            if(configure(algo).run(algorithm::BLOCK_SEARCH) !=
               algorithm::OPTIMAL)
                return verified{{}, "instance reported not optimal"};
            const auto cost = algo.template totalCost<total_cost>();
            if(const auto expected = reference_solution_value(min_file)) {
                if(static_cast<long long>(cost) != *expected)
                    return verified{{},
                                    "total cost " + std::to_string(cost) +
                                        " != reference " +
                                        std::to_string(*expected)};
            }
            checksum cs;
            cs.add(cost);
            return verified{cs.str(), {}};
        });
        if(!result.error.empty()) {
            state.SkipWithError(result.error);
            return;
        }
        state.SetLabel(result.label);

        for(auto _ : state) {
            algorithm algo(graph, _Mixing);
            configure(algo).run(algorithm::BLOCK_SEARCH);
            benchmark::DoNotOptimize(algo.template totalCost<total_cost>());
        }
    }
};

// Mixing is a benchmark parameter, like the value type: it changes how the
// same container solves the same instance, so it belongs after the value in
// the name rather than in the container's own dimension, where it would split
// one container into two series that no longer pair across libraries.
#define REGISTER(graph, value, mixing, tag)                                \
    benchmark::RegisterBenchmark(std::string(min_file.stem().c_str()) +    \
                                     "/" #graph ":network_simplex/" #value \
                                     "/" tag,                              \
                                 BM<graph, value, mixing>{}, min_file);

#define REGISTER_BOTH(graph, value)         \
    REGISTER(graph, value, true, "mixed")   \
    REGISTER(graph, value, false, "unmixed")

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [min_file, n, m] : instances) {
        REGISTER_BOTH(StaticDigraph, int)
        REGISTER_BOTH(SmartDigraph, int)
        REGISTER_BOTH(ListDigraph, int)
        REGISTER_BOTH(StaticDigraph, double)
        REGISTER_BOTH(SmartDigraph, double)
        REGISTER_BOTH(ListDigraph, double)
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

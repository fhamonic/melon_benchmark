#pragma once

// Dataset-independent body: the including .cpp provides the
// `instances` list (a <dataset>_instances.hpp include) before
// including this header.
//
// Min-cost-flow benchmarks.
//
// The graph dimension of each registered name is "<container>:<algorithm>"
// rather than just "<container>", so that MELON's network simplex and LEMON's
// land in the same chart -- the same convention the max-flow benchmarks use.
//
// network_simplex_default_traits is used with one field changed, arc_mixing:
// block search with block_size_factor 1.0 and min_block_size 10 stay as they
// are, which is the configuration the LEMON side is matched to. See the header
// comment there for the full list of what is and is not matched.
//
// Arc mixing is swept on both sides rather than fixed, because it is the
// single biggest difference between the two implementations and which way it
// cuts depends on the instance family. LEMON stores its internal copy of the
// arcs in a scattered order (stride max(m/n, 3)), so a block-search block
// holds arcs from many different source vertices; MELON has the same
// permutation as a *scan* order over the container's own arcs, off by default.
// Decorrelated blocks make better entering-arc choices, correlated ones make
// the reduced-cost gather cheaper:
//
//   .../mixed    the strided order, what LEMON's default gives you.
//   .../unmixed  the container's arc order, MELON's default.
//
// mutable_digraph is registered unmixed alone: a strided visit needs random
// access, and its arcs() is a join over the per-vertex out-arc lists, so MELON
// ignores the flag there and a "mixed" run would be the same measurement under
// a second name. The static_asserts below are what keeps that claim from going
// stale.

#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/algorithm/network_simplex.hpp"
#include "melon/container/mutable_digraph.hpp"
#include "melon/container/static_digraph.hpp"

using namespace melon;

#include "checksum.hpp"
#include "helper.hpp"
#include "parse_dimacs_min.hpp"
#include "reference_solution.hpp"

static_assert(std::ranges::random_access_range<arcs_range_t<static_digraph>>);
static_assert(!std::ranges::random_access_range<arcs_range_t<mutable_digraph>>);

// Capacities, supplies and costs are all _Value here, so the default traits'
// two type parameters coincide and only the flag under test differs.
template <typename _Value, bool _Mixing>
struct mixing_traits : network_simplex_default_traits<_Value, _Value> {
    static constexpr bool arc_mixing = _Mixing;
};

template <typename _Graph, typename _Value, bool _Mixing>
struct BM_network_simplex {
    void operator()(benchmark::State & state,
                    const std::filesystem::path & min_file) const {
        using traits = mixing_traits<_Value, _Mixing>;

        auto & [graph, capacities, costs, supplies] = cached_parse(
            min_file,
            [&] { return parse_dimacs_min<_Graph, _Value>(min_file); });

        // The reference check is a property of the instance, not of the
        // repetition, so it is cached together with the digest it guards.
        struct verified {
            std::string label;
            std::string error;
        };
        const auto & result = cached_setup(min_file, [&] {
            auto algo = network_simplex(traits{}, graph, capacities, costs,
                                        supplies);
            algo.run();
            if(algo.optimal())
                return verified{{}, "instance reported not optimal"};
            const auto cost = algo.total_cost();

            // Where the generator could certify the optimum in reasonable
            // time it ships a .sol next to the instance, and a benchmark
            // that does not check it is timing an unknown computation. The
            // largest instances of each family have none; there the gate is
            // validate.py's cross-library digest agreement.
            if(const auto expected = reference_solution_value(min_file)) {
                if(static_cast<long long>(cost) != *expected)
                    return verified{{},
                                    "total cost " + std::to_string(cost) +
                                        " != reference " +
                                        std::to_string(*expected)};
            }
            // The flow itself is not a canonical answer -- degenerate optima
            // let two correct implementations return different flows of the
            // same cost -- so the cost is what the digest is taken over. It
            // is quantized like every other value, so the int series checks
            // against the double series.
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
            auto algo =
                network_simplex(traits{}, graph, capacities, costs, supplies);
            algo.run();
            benchmark::DoNotOptimize(algo.total_cost());
        }
    }
};

// Mixing is a benchmark parameter, like the value type: it changes how the
// same container solves the same instance, so it belongs after the value in
// the name rather than in the container's own dimension, where it would split
// one container into two series that no longer pair across libraries.
#define REGISTER(graph, value, mixing, tag)                                 \
    benchmark::RegisterBenchmark(std::string(min_file.stem().c_str()) +     \
                                     "/" #graph ":network_simplex/" #value  \
                                     "/" tag,                               \
                                 BM_network_simplex<graph, value, mixing>{},\
                                 min_file);

#define REGISTER_BOTH(graph, value)         \
    REGISTER(graph, value, true, "mixed")   \
    REGISTER(graph, value, false, "unmixed")

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    for(const auto & [min_file, n, m] : instances) {
        REGISTER_BOTH(static_digraph, int)
        REGISTER(mutable_digraph, int, false, "unmixed")
        REGISTER_BOTH(static_digraph, double)
        REGISTER(mutable_digraph, double, false, "unmixed")
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

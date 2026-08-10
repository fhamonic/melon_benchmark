// CGAL surface-sweep on the same seeded segment sets as
// src/melon/bentley_ottmann/random.cpp; see there for the naming scheme.
//
// The two libraries' report_endpoints flags do NOT mean the same thing.
// melon's (true in its default traits) reports intersections that happen to
// lie at segment endpoints -- every reported event still involves >= 2
// segments. CGAL's true reports *every* endpoint, intersecting or not; its
// false setting reports exactly crossings plus endpoint touches, which is
// melon's semantics. Verified per-seed counts agree under (melon true, CGAL
// false) on all seeds; that is what the digests rely on.
//
// CGAL's API materializes the points into an output container rather than
// streaming them; the vector and its reserve are part of the measured cost
// because a caller cannot avoid them.

#include <cmath>
#include <format>
#include <map>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_sweep_2_algorithms.h>

#include "checksum.hpp"
#include "generate_segments.hpp"

using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
using Point_2 = Kernel::Point_2;
using Traits_2 = CGAL::Arr_segment_traits_2<Kernel>;
using Segment_2 = Traits_2::Curve_2;

constexpr int num_tests = 10;

std::size_t sweep(const std::vector<Segment_2> & segments, int num_segments) {
    std::vector<Point_2> intersections;
    intersections.reserve(
        static_cast<std::size_t>(std::pow(num_segments, 1.5)));
    CGAL::compute_intersection_points(segments.begin(), segments.end(),
                                      std::back_inserter(intersections),
                                      /* report_endpoints = */ false);
    return intersections.size();
}

void run_sweep(benchmark::State & state,
               const std::vector<std::vector<Segment_2>> & segments_vectors,
               int num_segments) {
    // Each --benchmark_repetitions run re-enters this function; the digest
    // pass costs as much as one timed iteration (tens of seconds at n=4096),
    // so it is computed once per benchmark name. Benchmarks run sequentially,
    // so the static map needs no lock.
    {
        static std::map<std::string, std::string> digest_cache;
        auto [it, inserted] = digest_cache.try_emplace(state.name());
        if(inserted) {
            checksum cs;
            for(auto && segments : segments_vectors)
                cs.add(sweep(segments, num_segments));
            it->second = cs.str();
        }
        state.SetLabel(it->second);
    }

    for(auto _ : state) {
        for(auto && segments : segments_vectors) {
            benchmark::DoNotOptimize(sweep(segments, num_segments));
        }
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * num_tests);
}

template <typename _Segments>
std::vector<Segment_2> to_cgal(const _Segments & segments) {
    std::vector<Segment_2> out;
    out.reserve(segments.size());
    for(auto && [a, b] : segments)
        out.emplace_back(Point_2(a.first, a.second), Point_2(b.first, b.second));
    return out;
}

template <int _Min, int _Max>
struct BM_box {
    int num_segments;
    void operator()(benchmark::State & state) const {
        std::vector<std::vector<Segment_2>> segments_vectors;
        for(int i = 0; i < num_tests; ++i) {
            segments_vectors.emplace_back(
                to_cgal(generate_random_box_segments<int, _Min, _Max>(
                    std::size_t(num_segments), i)));
        }
        run_sweep(state, segments_vectors, num_segments);
    }
};

template <int _Min, int _Max>
struct BM_vector {
    int num_segments;
    void operator()(benchmark::State & state) const {
        // Full box: the generator shrinks the anchor range itself (see the
        // melon side).
        constexpr int Length = (_Max - _Min) / 4;
        std::vector<std::vector<Segment_2>> segments_vectors;
        for(int i = 0; i < num_tests; ++i) {
            segments_vectors.emplace_back(
                to_cgal(generate_random_vector_segments<int, _Min, _Max,
                                                        Length>(
                    num_segments, i)));
        }
        run_sweep(state, segments_vectors, num_segments);
    }
};

template <int _Bits>
void register_family() {
    constexpr int Min = -(1 << (_Bits - 1));
    constexpr int Max = (1 << (_Bits - 1)) - 1;
    // Capped at 1024: CGAL's exact kernel needs tens of seconds per
    // iteration at n=4096, which priced the whole suite out of a casual
    // `make`; the asymptotic ordering is already visible at 1024.
    for(int n = 16; n <= 1024; n *= 4) {
        benchmark::RegisterBenchmark(
            std::format("box_n{}/epeck/{}bit", n, _Bits),
            BM_box<Min, Max>{n});
        benchmark::RegisterBenchmark(
            std::format("vec_n{}/epeck/{}bit", n, _Bits),
            BM_vector<Min, Max>{n});
    }
}

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);

    register_family<8>();
    register_family<16>();

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

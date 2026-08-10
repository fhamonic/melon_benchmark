// Bentley-Ottmann segment intersection on seeded random segment sets.
//
// Two families per size: "box_n<k>" draws both endpoints uniformly in the box
// (long segments, intersection-dense) and "vec_n<k>" draws an anchor plus a
// short vector (local segments, sparse intersections). The coordinate range is
// the task parameter: "8bit" packs everything into a 256x256 grid where
// degeneracies (collinear, shared endpoints) actually occur, "16bit" is the
// sparse regime. The container dimension is the coordinate value type; every
// type must produce the same digest, which is exactly the check that exact
// arithmetic in a smaller type is still exact.
//
// The digest is the per-seed count of reported event points, mixed over the 10
// seeded sets. CGAL is configured to report endpoint events too (see
// src/cgal/bentley_ottmann/random.cpp), so the counts are comparable.

#include <format>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/algorithm/bentley_ottmann.hpp"
#include "melon/numeric/bounded_value.hpp"
#include "melon/numeric/rational.hpp"

using namespace melon;
using numeric::bounded_value;
using numeric::integer;

#include "checksum.hpp"
#include "generate_segments.hpp"

// The default traits' report_endpoints=true reports intersections lying at
// segment endpoints (touches); every reported event involves >= 2 segments.
// See src/cgal/bentley_ottmann/random.cpp for how CGAL is matched to this.
constexpr int num_tests = 10;

template <typename _SegmentsVectors>
void run_bentley_ottmann(benchmark::State & state,
                         const _SegmentsVectors & segments_vectors,
                         int num_segments) {
    const auto segments_ids = std::views::iota(0, num_segments);

    // Each --benchmark_repetitions run re-enters this function; the digest
    // pass costs as much as one timed iteration (tens of seconds at n=4096),
    // so it is computed once per benchmark name. Benchmarks run sequentially,
    // so the static map needs no lock.
    {
        static std::map<std::string, std::string> digest_cache;
        auto [it, inserted] = digest_cache.try_emplace(state.name());
        if(inserted) {
            checksum cs;
            for(auto && segments : segments_vectors) {
                std::size_t num_events = 0;
                for(auto && [i, intersecting_segments] :
                    bentley_ottmann(segments_ids, segments)) {
                    ++num_events;
                }
                cs.add(num_events);
            }
            it->second = cs.str();
        }
        state.SetLabel(it->second);
    }

    for(auto _ : state) {
        for(auto && segments : segments_vectors) {
            std::size_t num_events = 0;
            for(auto && [i, intersecting_segments] :
                bentley_ottmann(segments_ids, segments)) {
                benchmark::DoNotOptimize(i);
                ++num_events;
            }
            benchmark::DoNotOptimize(num_events);
        }
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * num_tests);
}

template <typename _Value, int _Min, int _Max>
struct BM_box {
    int num_segments;
    void operator()(benchmark::State & state) const {
        using segments_vector =
            decltype(generate_random_box_segments<_Value, _Min, _Max>(
                std::size_t(num_segments), 0));
        std::vector<segments_vector> segments_vectors;
        for(int i = 0; i < num_tests; ++i) {
            segments_vectors.emplace_back(
                generate_random_box_segments<_Value, _Min, _Max>(
                    std::size_t(num_segments), i));
        }
        run_bentley_ottmann(state, segments_vectors, num_segments);
    }
};

template <typename _Value, int _Min, int _Max>
struct BM_vector {
    int num_segments;
    void operator()(benchmark::State & state) const {
        // The generator shrinks the anchor range by Length itself so that
        // endpoints stay inside [_Min, _Max]; pass the full box, or anchors
        // collapse into a tiny core (the original version shrank it twice).
        constexpr int Length = (_Max - _Min) / 4;
        using segments_vector =
            decltype(generate_random_vector_segments<_Value, _Min, _Max,
                                                     Length>(num_segments, 0));
        std::vector<segments_vector> segments_vectors;
        for(int i = 0; i < num_tests; ++i) {
            segments_vectors.emplace_back(
                generate_random_vector_segments<_Value, _Min, _Max, Length>(
                    num_segments, i));
        }
        run_bentley_ottmann(state, segments_vectors, num_segments);
    }
};

using bounded_int8_8 = bounded_value<int8_t>;
using bounded_int16_8 = bounded_value<int16_t, int16_t{-128}, int16_t{127}>;
using bounded_int32_8 = bounded_value<int32_t, int32_t{-128}, int32_t{127}>;

template <typename _Value, int _Bits>
void register_family(const char * type_name) {
    constexpr int Min = -(1 << (_Bits - 1));
    constexpr int Max = (1 << (_Bits - 1)) - 1;
    // Capped at 1024: CGAL's exact kernel needs tens of seconds per
    // iteration at n=4096, which priced the whole suite out of a casual
    // `make`; the asymptotic ordering is already visible at 1024.
    for(int n = 16; n <= 1024; n *= 4) {
        benchmark::RegisterBenchmark(
            std::format("box_n{}/{}/{}bit", n, type_name, _Bits),
            BM_box<_Value, Min, Max>{n});
        benchmark::RegisterBenchmark(
            std::format("vec_n{}/{}/{}bit", n, type_name, _Bits),
            BM_vector<_Value, Min, Max>{n});
    }
}

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);

    register_family<integer<bounded_int8_8>, 8>("bounded8");
    register_family<integer<bounded_int16_8>, 8>("bounded16");
    register_family<integer<bounded_int32_8>, 8>("bounded32");
    register_family<integer<int64_t>, 8>("int64");
    register_family<integer<__int128_t>, 8>("int128");

    register_family<integer<__int128_t>, 16>("int128");

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

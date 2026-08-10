#pragma once

#include <cstdint>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

// Seeded segment sets shared by every library's bentley_ottmann benchmark.
// Coordinates are drawn as raw ints before being converted to the caller's
// coordinate type, so melon's bounded/rational types and CGAL's kernel see the
// exact same segments for a given seed.
//
// Two constraints on the generated sets, both required for cross-library
// digests to be comparable:
//
// - No zero-length segments: CGAL's Arr_segment_traits_2 curves require
//   distinct endpoints.
// - No two segments share an endpoint. A shared endpoint is a point the two
//   sweeps *report differently*: melon reports it as an intersection (two
//   segments pass through it), while CGAL's compute_intersection_points either
//   omits it (report_endpoints=false skips endpoint-endpoint meetings) or
//   drowns it among every plain endpoint (report_endpoints=true). Crossings
//   and T-junctions (an endpoint interior to another segment) are reported by
//   both and remain in the sets.
//
// The seed goes through seed_seq because mt19937 seeded directly with small
// consecutive ints emits correlated first outputs. Note the anchor range of
// the vector family is shrunk by VEC_LENGTH *here*, so callers pass the full
// box; passing a pre-shrunk box shrinks it twice and collapses the anchors
// into a tiny core.

namespace segment_gen {

inline std::uint64_t pack_point(int x, int y) {
    return (std::uint64_t(std::uint32_t(x)) << 32) | std::uint32_t(y);
}

inline std::mt19937 seeded_rng(int seed) {
    std::seed_seq mix{seed};
    return std::mt19937(mix);
}

}  // namespace segment_gen

template <typename C, int BOX_MIN, int BOX_MAX>
auto generate_random_box_segments(std::size_t num_segments, int seed) {
    using point = std::pair<C, C>;
    using segment = std::pair<point, point>;
    std::vector<segment> segments;
    segments.reserve(num_segments);

    auto rng = segment_gen::seeded_rng(seed);
    std::uniform_int_distribution<int> dist(BOX_MIN, BOX_MAX);

    std::unordered_set<std::uint64_t> used_endpoints;
    while(segments.size() < num_segments) {
        const int x1 = dist(rng), y1 = dist(rng);
        const int x2 = dist(rng), y2 = dist(rng);
        if(x1 == x2 && y1 == y2) continue;
        const auto p1 = segment_gen::pack_point(x1, y1);
        const auto p2 = segment_gen::pack_point(x2, y2);
        if(used_endpoints.contains(p1) || used_endpoints.contains(p2)) continue;
        used_endpoints.insert(p1);
        used_endpoints.insert(p2);
        segments.emplace_back(point(x1, y1), point(x2, y2));
    }
    return segments;
}

template <typename C, int BOX_MIN, int BOX_MAX, int VEC_LENGTH>
auto generate_random_vector_segments(std::size_t num_segments, int seed) {
    using point = std::pair<C, C>;
    using segment = std::pair<point, point>;
    std::vector<segment> segments;
    segments.reserve(num_segments);

    auto rng = segment_gen::seeded_rng(seed);
    std::uniform_int_distribution<int> box_dist(BOX_MIN + VEC_LENGTH,
                                                BOX_MAX - VEC_LENGTH);
    std::uniform_int_distribution<int> vec_dist(-VEC_LENGTH, VEC_LENGTH);

    std::unordered_set<std::uint64_t> used_endpoints;
    while(segments.size() < num_segments) {
        const int a = box_dist(rng), b = box_dist(rng);
        const int dx = vec_dist(rng), dy = vec_dist(rng);
        if(dx == 0 && dy == 0) continue;
        const auto p1 = segment_gen::pack_point(a, b);
        const auto p2 = segment_gen::pack_point(a + dx, b + dy);
        if(used_endpoints.contains(p1) || used_endpoints.contains(p2)) continue;
        used_endpoints.insert(p1);
        used_endpoints.insert(p2);
        segments.emplace_back(point(a, b), point(a + dx, b + dy));
    }
    return segments;
}

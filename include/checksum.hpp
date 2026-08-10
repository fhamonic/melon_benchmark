#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <string>
#include <vector>

// Digest of an algorithm's result, published as the Google Benchmark label of
// the run.
//
// Every benchmark feeds its result into a checksum in a *canonical* order --
// vertex id order, never traversal order -- so that two libraries that agree
// on the answer produce the same digest even though they discover it in
// different orders. `validate.py` then asserts that all libraries and all
// graph containers agree on every (algorithm, dataset, instance) triple.
//
// This is the only thing standing between the plots and the classic failure
// mode of library benchmarks: a baseline that is quietly solving an easier
// problem, and therefore winning or losing for reasons that have nothing to
// do with its implementation quality.
class checksum {
    std::uint64_t _h = 14695981039346656037ull;  // FNV-1a 64-bit offset basis

    constexpr void _mix(std::uint64_t v) noexcept {
        for(int i = 0; i < 8; ++i) {
            _h = (_h ^ ((v >> (i * 8)) & 0xffull)) * 1099511628211ull;
        }
    }

    // Everything is quantized to 1/1024 before mixing, integral or not.
    //
    // Two consequences, both wanted. Values are compared with a tolerance, so
    // two libraries that break a tie differently -- and therefore sum the same
    // total along a different path, in a different order -- still agree.  And
    // `5` and `5.0` produce the same digest, so validation can cross-check the
    // `int` series of an algorithm against its `double` series. That is
    // exactly the check that catches a distance map declared with the wrong
    // value type, which is how Boost's `double` Dijkstra was silently running
    // on truncated integer arithmetic.
    //
    // 1024 is a power of two, so the integral weights that every shipped
    // dataset contains survive the scaling exactly.
    static constexpr double scale = 1024.0;

public:
    template <std::integral T>
    constexpr void add(T v) noexcept {
        _mix(static_cast<std::uint64_t>(static_cast<std::int64_t>(v) * 1024));
    }

    template <std::floating_point T>
    constexpr void add(T v) noexcept {
        _mix(static_cast<std::uint64_t>(
            std::llround(static_cast<double>(v) * scale)));
    }

    // Distinct sentinel for "no value here" (unreachable vertex, absent
    // predecessor), so that a library reporting an unreachable vertex cannot
    // collide with one reporting a real distance.
    constexpr void add_unreached() noexcept { _mix(0xDEADBEEFCAFEBABEull); }

    [[nodiscard]] std::string str() const {
        static constexpr char digits[] = "0123456789abcdef";
        std::string s(16, '0');
        for(int i = 0; i < 16; ++i)
            s[15 - i] = digits[(_h >> (i * 4)) & 0xfull];
        return s;
    }
};

// Canonical digest of a vertex partition (connected components, strongly
// connected components, traversal forest).
//
// Component *ids* are an implementation detail -- libraries number them in
// whatever order they happen to discover them. What is canonical is, for each
// vertex in id order, the smallest vertex id sharing its component. Two
// libraries agree on the partition iff they agree on this digest.
template <typename _ComponentOf>
[[nodiscard]] std::string partition_checksum(std::size_t num_vertices,
                                             _ComponentOf && component_of) {
    // representative[c] = smallest vertex id seen in component c
    std::vector<std::size_t> representative;
    std::vector<std::size_t> component(num_vertices);

    for(std::size_t u = 0; u < num_vertices; ++u) {
        const std::size_t c = static_cast<std::size_t>(component_of(u));
        component[u] = c;
        if(c >= representative.size())
            representative.resize(c + 1, num_vertices);
        representative[c] = std::min(representative[c], u);
    }

    checksum cs;
    for(std::size_t u = 0; u < num_vertices; ++u)
        cs.add(representative[component[u]]);
    return cs.str();
}

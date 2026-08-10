#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <ranges>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "melon/container/static_filter_map.hpp"
#include "melon/container/static_map.hpp"

#include "checksum.hpp"

// static_filter_map against the two containers a user would reach for
// instead: static_map<K, bool> (one byte per key, what create_vertex_map<bool>
// vends) and std::vector<bool> (bit-packed, but no word access, so no
// bit-scanning enumeration).
//
// The workloads separate the two regimes that decide the comparison:
//
//  - random access ("visited map" pattern): byte maps win while they fit in
//    cache, bit maps win once the byte map falls out of it. This is the only
//    workload melon's algorithms put a bool map through, hence the two sizes:
//    64K keys keeps the byte map cache-resident, 4M keys does not.
//
//  - enumeration of the set keys ("filter" pattern): static_filter_map's
//    filter() skips 64 keys per word via countr_zero, which neither
//    alternative can express. The byte-map and vector<bool> series scan
//    naively. Two series, because the result depends on what consumes the
//    enumerated keys. "scan" escapes every key through DoNotOptimize -- the
//    consumer is opaque (queue push, callback), which is what algorithms do
//    with enumerated keys; the naive scans then run branchy and filter()
//    wins at every density. "scan_reduce" only escapes an accumulated sum --
//    the consumer folds into the scan, gcc and clang vectorize the byte
//    scan into a density-independent SIMD sweep, and filter() only wins
//    below ~50% density. filter()'s own walk is sequential and branchy
//    either way, so it times identically in both series.
//
// Names follow "<instance>/<container>": the instance encodes workload, size
// and density, so validate.py cross-checks that the three containers publish
// identical digests for every task.

using filter_map = melon::static_filter_map<unsigned int>;
using bool_map = melon::static_map<unsigned int, bool>;
using bool_vector = std::vector<bool>;

template <typename _Map>
inline constexpr const char * map_name = nullptr;
template <>
inline constexpr const char * map_name<filter_map> = "static_filter_map";
template <>
inline constexpr const char * map_name<bool_map> = "static_map<bool>";
template <>
inline constexpr const char * map_name<bool_vector> = "vector<bool>";

template <typename _Map>
void fill_map(_Map & map, bool b) {
    if constexpr(requires { map.fill(b); })
        map.fill(b);
    else
        std::fill(map.begin(), map.end(), b);
}

// Sum of the set keys, each container through its natural interface:
// filter() for static_filter_map, a naive scan for the other two.
template <typename _Map>
[[nodiscard]] std::uint64_t scan_sum(const _Map & map, unsigned int n) {
    std::uint64_t sum = 0;
    if constexpr(std::same_as<_Map, filter_map>) {
        for(unsigned int k : map.filter(std::views::iota(0u, n))) sum += k;
    } else {
        for(unsigned int k = 0; k < n; ++k)
            if(map[k]) sum += k;
    }
    return sum;
}

// Digest of the map contents in key order, so that all three containers must
// agree on *which* keys ended up set, not just on how many.
template <typename _Map>
[[nodiscard]] std::string contents_checksum(const _Map & map, unsigned int n) {
    checksum cs;
    for(unsigned int k = 0; k < n; ++k) cs.add(static_cast<int>(bool(map[k])));
    return cs.str();
}

// A shuffled permutation of [0, n): every key touched exactly once.
[[nodiscard]] std::vector<unsigned int> shuffled_keys(unsigned int n) {
    std::vector<unsigned int> keys(n);
    std::iota(keys.begin(), keys.end(), 0u);
    std::mt19937 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);
    return keys;
}

// n draws with replacement: the repeated-checks profile of a visited map.
[[nodiscard]] std::vector<unsigned int> random_keys(unsigned int n) {
    std::vector<unsigned int> keys(n);
    std::mt19937 rng(43);
    std::uniform_int_distribution<unsigned int> draw(0u, n - 1u);
    for(auto & k : keys) k = draw(rng);
    return keys;
}

template <typename _Map>
struct random_write {
    void operator()(benchmark::State & state, unsigned int n) const {
        const auto keys = shuffled_keys(n);
        // Half the permutation, so the digest depends on which keys were
        // written, not only on their count.
        const std::size_t writes = n / 2;
        _Map map(n, false);
        for(std::size_t i = 0; i < writes; ++i) map[keys[i]] = true;
        state.SetLabel(contents_checksum(map, n));
        for(auto _ : state) {
            for(std::size_t i = 0; i < writes; ++i) map[keys[i]] = true;
            benchmark::ClobberMemory();
        }
        state.SetItemsProcessed(int64_t(state.iterations()) *
                                int64_t(writes));
    }
};

template <typename _Map>
struct random_read {
    void operator()(benchmark::State & state, unsigned int n) const {
        const auto keys = random_keys(n);
        _Map map(n, false);
        for(unsigned int k = 0; k < n; k += 3) map[k] = true;
        const _Map & cmap = map;
        checksum cs;
        std::uint64_t expected = 0;
        for(unsigned int k : keys) expected += bool(cmap[k]);
        cs.add(expected);
        state.SetLabel(cs.str());
        for(auto _ : state) {
            std::uint64_t sum = 0;
            for(unsigned int k : keys) sum += bool(cmap[k]);
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
    }
};

// The exact loop of a traversal's reached-map: reset, then check-then-set
// with repeated hits. The reset is timed with it on purpose -- an algorithm
// that reuses its map pays it on every run.
template <typename _Map>
struct check_then_set {
    void operator()(benchmark::State & state, unsigned int n) const {
        const auto keys = random_keys(n);
        _Map map(n, false);
        const auto run = [&]() {
            fill_map(map, false);
            std::uint64_t first_visits = 0;
            for(unsigned int k : keys) {
                if(!map[k]) {
                    map[k] = true;
                    ++first_visits;
                }
            }
            return first_visits;
        };
        checksum cs;
        cs.add(run());
        state.SetLabel(cs.str() + contents_checksum(map, n));
        for(auto _ : state) {
            benchmark::DoNotOptimize(run());
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
    }
};

template <typename _Map>
[[nodiscard]] _Map make_scan_map(unsigned int n, int density_pct) {
    _Map map(n, false);
    std::mt19937 rng(7);
    std::bernoulli_distribution coin(density_pct / 100.0);
    for(unsigned int k = 0; k < n; ++k) map[k] = coin(rng);
    return map;
}

// Opaque consumer: every enumerated key escapes through DoNotOptimize, so
// the naive scans cannot be folded into a SIMD sweep.
template <typename _Map>
struct scan {
    void operator()(benchmark::State & state, unsigned int n,
                    int density_pct) const {
        const _Map map = make_scan_map<_Map>(n, density_pct);
        checksum cs;
        cs.add(scan_sum(map, n));
        state.SetLabel(cs.str());
        for(auto _ : state) {
            if constexpr(std::same_as<_Map, filter_map>) {
                for(unsigned int k : map.filter(std::views::iota(0u, n)))
                    benchmark::DoNotOptimize(k);
            } else {
                for(unsigned int k = 0; k < n; ++k)
                    if(map[k]) benchmark::DoNotOptimize(k);
            }
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
    }
};

// Foldable consumer: only the accumulated sum escapes, so the compiler may
// vectorize the naive scans -- the byte map's best case.
template <typename _Map>
struct scan_reduce {
    void operator()(benchmark::State & state, unsigned int n,
                    int density_pct) const {
        const _Map map = make_scan_map<_Map>(n, density_pct);
        checksum cs;
        cs.add(scan_sum(map, n));
        state.SetLabel(cs.str());
        for(auto _ : state) {
            benchmark::DoNotOptimize(scan_sum(map, n));
        }
        state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
    }
};

template <typename _Map>
struct fill_all {
    void operator()(benchmark::State & state, unsigned int n) const {
        _Map map(n, false);
        bool b = true;
        for(auto _ : state) {
            fill_map(map, b);
            benchmark::ClobberMemory();
            b = !b;
        }
        state.SetLabel("-");
        state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
    }
};

template <typename _Map>
void register_benchmarks(unsigned int n, const std::string & size_tag) {
    const std::string suffix =
        "-" + size_tag + "/" + std::string(map_name<_Map>);
    benchmark::RegisterBenchmark("random_write" + suffix,
                                 random_write<_Map>{}, n);
    benchmark::RegisterBenchmark("random_read" + suffix, random_read<_Map>{},
                                 n);
    benchmark::RegisterBenchmark("check_then_set" + suffix,
                                 check_then_set<_Map>{}, n);
    for(int density_pct : {1, 10, 50, 90}) {
        benchmark::RegisterBenchmark(
            "scan" + std::to_string(density_pct) + suffix, scan<_Map>{}, n,
            density_pct);
        benchmark::RegisterBenchmark(
            "scan_reduce" + std::to_string(density_pct) + suffix,
            scan_reduce<_Map>{}, n, density_pct);
    }
    benchmark::RegisterBenchmark("fill" + suffix, fill_all<_Map>{}, n);
}

int main(int argc, char ** argv) {
    benchmark::MaybeReenterWithoutASLR(argc, argv);
    // 64K keys: the byte map (64 KiB) is cache-resident, its best case.
    // 4M keys: the byte map (4 MiB) is not, the bit map (512 KiB) still is.
    for(const auto & [n, size_tag] :
        {std::pair{1u << 16, "64K"}, std::pair{1u << 22, "4M"}}) {
        register_benchmarks<filter_map>(n, size_tag);
        register_benchmarks<bool_map>(n, size_tag);
        register_benchmarks<bool_vector>(n, size_tag);
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}

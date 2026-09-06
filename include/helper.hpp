#pragma once

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// Evenly spaced source vertices, sized so that the total work is roughly a
// constant complexity budget and small and large instances therefore cost
// comparable wall time.
//
// The absolute cap matters for tiny instances: the budget alone would give
// rome99 (3353 vertices) over a thousand sources, which buys no extra
// stability and which the slowest library benchmarked here cannot finish. It
// binds only on instances that small -- every USA road network already lands
// well under it.
static constexpr int max_sources_per_instance = 16;

template <typename _Complexity>
const auto instance_sources(int num_vertices, int num_arcs,
                            _Complexity && complexity) {
    const int max_num_sources = std::min(
        max_sources_per_instance,
        1 + static_cast<int>(1e6 / complexity(num_vertices, num_arcs)));
    const int incr = num_vertices / std::min(num_vertices, max_num_sources);

    std::vector<unsigned int> sources;
    for(int i = 0; i < num_vertices; i += incr) sources.emplace_back(i);
    return sources;
}

// Target paired with each source for point-to-point queries. Half the id range
// away, so on a road network the query is a genuinely long one rather than a
// walk to a neighbour. Deterministic, so every library asks the same question.
[[nodiscard]] inline unsigned int instance_target(unsigned int source,
                                                  int num_vertices) {
    return static_cast<unsigned int>(
        (source + static_cast<unsigned int>(num_vertices) / 2u) %
        static_cast<unsigned int>(num_vertices));
}

// An instance a single run cannot afford, whatever the number of sources.
//
// instance_sources spends a complexity budget on how many sources to start
// from, but it can never go below one, so for an algorithm whose single run is
// already enormous the budget stops binding. Bellman-Ford is where that shows:
// one run over USA-road-t.NW costs up to 22 s against 0.12 s for Dijkstra on
// the same graph, and the four Bellman-Ford binaries accounted for three
// quarters of all the work the suite actually measures. The three smaller road
// networks span 264k to 436k vertices and already order the libraries and the
// containers the same way.
//
// The bound is on the algorithm's own complexity expression, so each benchmark
// is judged by the work *it* does rather than by the size of the file.
static constexpr double max_instance_complexity = 1e12;

[[nodiscard]] inline bool instance_is_affordable(double complexity) {
    return complexity <= max_instance_complexity;
}

// Google Benchmark calls a registered benchmark function once per repetition,
// and once more before those while it calibrates the iteration count.
// Everything a benchmark body does before `for(auto _ : state)` therefore runs
// repetitions+1 times -- and that prologue parses an up-to-84 MB DIMACS file
// and runs the reference pass whose digest becomes the run's label. Both cost
// more than the measurement they precede, so left alone they were the largest
// single item in the suite's wall time.
//
// Neither depends on anything but the instance, so both are memoized below.
// Each cache holds one entry: benchmarks are registered instance by instance,
// so consecutive calls ask for the same file, and holding every parsed road
// network at once would cost gigabytes.
namespace bench_cache {
template <typename _T>
struct slot {
    static inline std::filesystem::path key;
    static inline std::unique_ptr<_T> value;
};
}  // namespace bench_cache

// The parsed instance, built at most once per (type, file) in a binary.
//
// Keyed by the *type* the parser returns rather than by the call site, so the
// twelve melon Dijkstra variants of one graph share four parses -- one per
// container and value type -- instead of doing twelve. That is sound because a
// parse result is a pure function of its type and its file, unlike a label,
// which is whatever its own benchmark computed.
//
// The reference handed back is non-const because several baselines take their
// graph by non-const reference. Sharing it across repetitions is safe for the
// same reason it is already safe across iterations: every timed loop here
// runs its algorithm many times over one parsed graph, so an algorithm that
// left state behind in its input would already be measured wrong.
template <typename _Parse>
[[nodiscard]] auto & cached_parse(const std::filesystem::path & file,
                                  _Parse && parse) {
    using instance_type = std::invoke_result_t<_Parse &>;
    using cache = bench_cache::slot<instance_type>;
    if(cache::value && cache::key == file) return *cache::value;
    // Released before the replacement is parsed: one road network alive at a
    // time per type, rather than two.
    cache::value.reset();
    // `new T(parse())` rather than make_unique: the prvalue initializes the
    // object directly, so the instance is never moved. Boost's adjacency_list
    // does not survive being moved, and LEMON's graphs cannot be moved at all.
    cache::value.reset(new instance_type(parse()));
    cache::key = file;
    return *cache::value;
}

// Same, for parsers that fill a default-constructed instance through a
// reference because their graph is neither copyable nor movable -- LEMON's are
// not, and its ArcMaps hold a pointer to the graph they were built from.
template <typename _Instance, typename _Parse>
[[nodiscard]] _Instance & cached_parse_into(const std::filesystem::path & file,
                                            _Parse && parse) {
    using cache = bench_cache::slot<_Instance>;
    if(cache::value && cache::key == file) return *cache::value;
    cache::value.reset();
    auto fresh = std::make_unique<_Instance>();
    parse(*fresh);
    cache::key = file;
    cache::value = std::move(fresh);
    return *cache::value;
}

// Whatever else the prologue produces -- almost always the label a benchmark
// publishes, its result digest (see checksum.hpp), sometimes paired with the
// reference check that decides whether the digest is worth publishing at all.
//
// Keyed by the call site: the local class below is a distinct type in every
// instantiation, and every lambda passed in has its own type. Labels cannot be
// shared the way a parse can, because two benchmarks in one binary digesting
// the same file are digesting *different* results.
template <typename _Compute>
[[nodiscard]] const auto & cached_setup(const std::filesystem::path & file,
                                        _Compute && compute) {
    struct held {
        std::invoke_result_t<_Compute &> value;
    };
    using cache = bench_cache::slot<held>;
    if(!cache::value || cache::key != file) {
        cache::value.reset(new held{compute()});
        cache::key = file;
    }
    return cache::value->value;
}

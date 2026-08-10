#pragma once

#include <algorithm>
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
        1 + static_cast<int>(1e8 / complexity(num_vertices, num_arcs)));
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
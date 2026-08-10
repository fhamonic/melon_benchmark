#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "checksum.hpp"

// Boost vertex descriptors are the integers 0..num_vertices-1 for both
// containers benchmarked here (adjacency_list with vecS, and
// compressed_sparse_row), so plain index order is already the canonical order
// the digests require.

// Digest of one shortest-path run. Boost initializes unreached vertices to
// the maximum value of the distance type rather than leaving them unset.
template <typename _Value>
void boost_add_distances(checksum & cs, const std::vector<_Value> & distances) {
    constexpr _Value unreached = (std::numeric_limits<_Value>::max)();
    for(const _Value & d : distances) {
        if(d == unreached)
            cs.add_unreached();
        else
            cs.add(d);
    }
}

// Digest of the reachable set of one traversal, in vertex index order.
inline void boost_add_reachability(checksum & cs,
                                   const std::vector<char> & reached) {
    for(const char r : reached) cs.add(r ? 1 : 0);
}

// Canonical digest of a partition held in a component-index vector.
[[nodiscard]] inline std::string boost_partition_checksum(
    const std::vector<int> & component) {
    return partition_checksum(component.size(), [&](std::size_t u) {
        return static_cast<std::size_t>(component[u]);
    });
}

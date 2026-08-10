#pragma once

#include <cstddef>
#include <string>

#include <lemon/core.h>

#include "checksum.hpp"

// LEMON node ids are contiguous over 0..countNodes()-1 for StaticDigraph,
// SmartDigraph and ListDigraph as they are built here, so nodeFromId walks
// them in the canonical order the digests require.
//
// These take the already-run algorithm's accessors as callables rather than
// the algorithm itself: LEMON algorithm objects own the maps they allocate,
// so handing them around by value is not safe.

// Appends the reachable set of one traversal, in node id order.
template <typename _Graph, typename _Reached>
void lemon_add_reachability(const _Graph & graph, checksum & cs,
                            _Reached && reached) {
    const int n = lemon::countNodes(graph);
    for(int i = 0; i < n; ++i) cs.add(reached(graph.nodeFromId(i)) ? 1 : 0);
}

// Appends the distances of one shortest-path run, in node id order.
template <typename _Graph, typename _Reached, typename _Dist>
void lemon_add_distances(const _Graph & graph, checksum & cs,
                         _Reached && reached, _Dist && dist) {
    const int n = lemon::countNodes(graph);
    for(int i = 0; i < n; ++i) {
        const auto u = graph.nodeFromId(i);
        if(reached(u))
            cs.add(dist(u));
        else
            cs.add_unreached();
    }
}

// Canonical digest of a partition held in a LEMON NodeMap<int>.
template <typename _Graph, typename _CompMap>
[[nodiscard]] std::string lemon_partition_checksum(const _Graph & graph,
                                                   const _CompMap & comp) {
    return partition_checksum(
        static_cast<std::size_t>(lemon::countNodes(graph)), [&](std::size_t u) {
            return static_cast<std::size_t>(
                comp[graph.nodeFromId(static_cast<int>(u))]);
        });
}

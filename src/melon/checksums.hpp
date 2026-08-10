#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "checksum.hpp"

#include "melon/graph.hpp"

// Every digest here iterates vertex *ids*, never melon::vertices(graph):
// mutable_digraph walks an intrusive list and yields vertices in reverse
// creation order, which would make the digest depend on the container rather
// than on the answer.

// Digest of the set of vertices reached from each source, in id order.
// `run(source)` returns the traversal range of the algorithm under test.
template <typename _Graph, typename _Run>
[[nodiscard]] std::string melon_traversal_checksum(
    const _Graph & graph, const std::vector<unsigned int> & sources,
    _Run && run) {
    const std::size_t n = melon::num_vertices(graph);
    std::vector<char> reached(n);
    checksum cs;
    for(auto && s : sources) {
        std::fill(reached.begin(), reached.end(), char{0});
        for(auto && v : run(s)) reached[static_cast<std::size_t>(v)] = 1;
        for(std::size_t i = 0; i < n; ++i) cs.add(reached[i]);
    }
    return cs.str();
}

// Digest of a vertex partition delivered as a range of ranges of vertices.
// Component numbering is an implementation detail, so partition_checksum
// canonicalizes it before hashing.
template <typename _Graph, typename _Components>
[[nodiscard]] std::string melon_partition_checksum(const _Graph & graph,
                                                   _Components && components) {
    const std::size_t n = melon::num_vertices(graph);
    std::vector<std::size_t> component(n, 0);
    std::size_t c = 0;
    for(auto && comp : components) {
        for(auto && v : comp) component[static_cast<std::size_t>(v)] = c;
        ++c;
    }
    return partition_checksum(n, [&](std::size_t u) { return component[u]; });
}

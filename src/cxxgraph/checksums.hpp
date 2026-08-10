#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "CXXGraph/CXXGraph.hpp"

#include "checksum.hpp"

// CXXGraph traversals return the visited nodes as a vector, in traversal
// order. The digest has to be in vertex id order like every other library's,
// so the reached set is scattered back into an index-ordered buffer first --
// each node carries its 0-based DIMACS index as its payload.
template <typename _Nodes>
void cxxgraph_add_reachability(checksum & cs, std::size_t num_vertices,
                               const _Nodes & visited) {
    std::vector<char> reached(num_vertices, 0);
    for(const auto & node : visited)
        reached[static_cast<std::size_t>(node.getData())] = 1;
    for(const char r : reached) cs.add(r ? 1 : 0);
}

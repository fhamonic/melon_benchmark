/*
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.*/
/**
 * @file all.hpp
 * @author François Hamonic (francois.hamonic@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-01-02
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef FHAMONIC_MELON_HPP
#define FHAMONIC_MELON_HPP

#ifndef MELON_STATIC_DIGRAPH_HPP
#define MELON_STATIC_DIGRAPH_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ranges>
#include <vector>

namespace fhamonic {
namespace melon {

class StaticDigraph {
public:
    using Node = unsigned int;
    using Arc = unsigned int;

    template <typename T>
    using NodeMap = std::vector<T>;
    template <typename T>
    using ArcMap = std::vector<T>;

private:
    std::vector<Arc> out_arc_begin;
    std::vector<Node> arc_target;

public:
    StaticDigraph(std::vector<Arc> && begins, std::vector<Node> && targets)
        : out_arc_begin(std::move(begins)), arc_target(std::move(targets)) {}

    StaticDigraph() = default;
    StaticDigraph(const StaticDigraph & graph) = default;
    StaticDigraph(StaticDigraph && graph) = default;

    auto nb_nodes() const { return out_arc_begin.size(); }
    auto nb_arcs() const { return arc_target.size(); }

    bool is_valid_node(Node u) const { return u < nb_nodes(); }
    bool is_valid_arc(Arc u) const { return u < nb_arcs(); }

    auto nodes() const {
        return std::views::iota(static_cast<Node>(0),
                                static_cast<Node>(nb_nodes()));
    }
    auto arcs() const {
        return std::views::iota(static_cast<Arc>(0),
                                static_cast<Arc>(nb_arcs()));
    }
    auto out_arcs(const Node u) const {
        assert(is_valid_node(u));
        return std::views::iota(
            out_arc_begin[u],
            (u + 1 < nb_nodes() ? out_arc_begin[u + 1] : nb_arcs()));
    }
    Node source(Arc a) const {  // O(\log |V|)
        assert(is_valid_arc(a));
        auto it =
            std::ranges::lower_bound(out_arc_begin, a, std::less_equal<Arc>());
        return static_cast<Node>(std::distance(out_arc_begin.begin(), --it));
    }
    Node target(Arc a) const {
        assert(is_valid_arc(a));
        return arc_target[a];
    }
    auto out_neighbors(const Node u) const {
        assert(is_valid_node(u));
        return std::ranges::subrange(
            arc_target.begin() + out_arc_begin[u],
            (u + 1 < nb_nodes() ? arc_target.begin() + out_arc_begin[u + 1]
                                : arc_target.end()));
    }

    auto out_arcs_pairs(const Node u) const {
        assert(is_valid_node(u));
        return std::views::transform(
            out_neighbors(u), [u](auto v) { return std::make_pair(u, v); });
    }
    auto arcs_pairs() const {
        return std::views::join(std::views::transform(
            nodes(), [this](auto u) { return out_arcs_pairs(u); }));
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_STATIC_DIGRAPH_HPP
#ifndef MELON_STATIC_DIGRAPH_BUILDER_HPP
#define MELON_STATIC_DIGRAPH_BUILDER_HPP

#include <algorithm>
#include <numeric>
#include <ranges>
#include <vector>

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

namespace fhamonic {
namespace melon {

template <typename... ArcProperty>
class StaticDigraphBuilder {
public:
    using Node = StaticDigraph::Node;
    using Arc = StaticDigraph::Arc;

    using PropertyMaps = std::tuple<std::vector<ArcProperty>...>;

private:
    std::size_t _nb_nodes;
    std::vector<Arc> nb_out_arcs;
    std::vector<Node> arc_sources;
    std::vector<Node> arc_targets;
    PropertyMaps arc_property_maps;

public:
    StaticDigraphBuilder() : _nb_nodes(0) {}
    StaticDigraphBuilder(std::size_t nb_nodes)
        : _nb_nodes(nb_nodes), nb_out_arcs(nb_nodes, 0) {}

private:
    template <class Maps, class Properties, std::size_t... Is>
    void addProperties(Maps && maps, Properties && properties,
                       std::index_sequence<Is...>) {
        (get<Is>(maps).push_back(get<Is>(properties)), ...);
    }

public:
    void addArc(Node u, Node v, ArcProperty... properties) {
        assert(_nb_nodes > std::max(u, v));
        ++nb_out_arcs[u];
        arc_sources.push_back(u);
        arc_targets.push_back(v);
        addProperties(
            arc_property_maps, std::make_tuple(properties...),
            std::make_index_sequence<std::tuple_size<PropertyMaps>{}>{});
    }

    auto build() {
        // sort arc_sources, arc_tagrets and arc_property_maps
        auto arcs_zipped_view = std::apply(
            [this](auto &&... property_map) {
                return ranges::view::zip(arc_sources, arc_targets,
                                         property_map...);
            },
            arc_property_maps);
        ranges::sort(arcs_zipped_view, [](const auto & a, const auto & b) {
            if(std::get<0>(a) == std::get<0>(b))
                return std::get<1>(a) < std::get<1>(b);
            return std::get<0>(a) < std::get<0>(b);
        });
        // compute out_arc_begin
        std::exclusive_scan(nb_out_arcs.begin(), nb_out_arcs.end(),
                            nb_out_arcs.begin(), 0);
        // create graph
        StaticDigraph graph(std::move(nb_out_arcs), std::move(arc_targets));
        return std::apply(
            [this, &graph](auto &&... property_map) {
                return std::make_tuple(graph, property_map...);
            },
            arc_property_maps);
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_STATIC_DIGRAPH_BUILDER_HPP

#ifndef MELON_DIJKSTRA_HPP
#define MELON_DIJKSTRA_HPP

#include <algorithm>
#include <queue>
#include <ranges>
#include <vector>

#ifndef MELON_BINARY_HEAP_HPP
#define MELON_BINARY_HEAP_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace fhamonic {
namespace melon {

template <typename ND, typename PR, typename CMP = std::less<PR>>
class BinaryHeap {
public:
    using Node = ND;
    using Prio = PR;
    using Compare = CMP;
    using Pair = std::pair<Node, Prio>;

private:
    using Difference = std::size_t;

public:
    enum State {
        PRE_HEAP = Difference(0),
        POST_HEAP = Difference(1),
        IN_HEAP = Difference(sizeof(Pair))
    };

    std::vector<Pair> heap_array;
    std::byte * heap_ptr;
    std::vector<Difference> indices_map;
    Compare cmp;

public:
    BinaryHeap(const std::size_t nb_nodes)
        : heap_array(), indices_map(nb_nodes, State::PRE_HEAP), cmp() {}

    BinaryHeap(const BinaryHeap & bin) = default;
    BinaryHeap(BinaryHeap && bin) = default;

    int size() const noexcept { return heap_array.size(); }
    bool empty() const noexcept { return heap_array.empty(); }
    void clear() noexcept {
        heap_array.clear();
        std::ranges::fill(indices_map, State::PRE_HEAP);
    }

private:
    constexpr Pair & pair_ref(Difference i) {
        return *(reinterpret_cast<Pair *>(heap_ptr + i));
    }
    constexpr const Pair & pair_ref(Difference i) const {
        return *(reinterpret_cast<Pair *>(heap_ptr + i));
    }

    void heap_move(Difference index, Pair && p) noexcept {
        indices_map[p.first] = index;
        pair_ref(index) = std::move(p);
    }

    void heap_push(Difference holeIndex, Pair && p) noexcept {
        Difference parent = holeIndex / (2 * sizeof(Pair)) * sizeof(Pair);
        while(holeIndex > sizeof(Pair) &&
              cmp(p.second, pair_ref(parent).second)) {
            heap_move(holeIndex, std::move(pair_ref(parent)));
            holeIndex = parent;
            parent = holeIndex / (2 * sizeof(Pair)) * sizeof(Pair);
        }
        heap_move(holeIndex, std::move(p));
    }

    void adjust_heap(Difference holeIndex, const Difference len,
                     Pair && p) noexcept {
        Difference child = 2 * holeIndex;
        while(child < len) {
            child += sizeof(Pair) * cmp(pair_ref(child + sizeof(Pair)).second,
                                        pair_ref(child).second);
            if(!cmp(pair_ref(child).second, p.second)) {
                return heap_move(holeIndex, std::move(p));
            }
            heap_move(holeIndex, std::move(pair_ref(child)));
            holeIndex = child;
            child = 2 * holeIndex;
        }
        if(child < len && cmp(pair_ref(child).second, p.second)) {
            heap_move(holeIndex, std::move(pair_ref(child)));
            holeIndex = child;
        }
        heap_move(holeIndex, std::move(p));
    }

public:
    void push(Pair && p) noexcept {
        heap_array.emplace_back();
        heap_ptr = reinterpret_cast<std::byte *>(heap_array.data() - 1);
        heap_push(Difference(heap_array.size() * sizeof(Pair)), std::move(p));
    }
    void push(const Node i, const Prio p) noexcept { push(Pair(i, p)); }
    bool contains(const Node u) const noexcept { return indices_map[u] > 0; }
    Prio prio(const Node u) const noexcept {
        return pair_ref(indices_map[u]).second;
    }
    Pair top() const noexcept { return heap_array.front(); }
    Pair pop() noexcept {
        assert(!heap_array.empty());
        const Difference n = Difference(heap_array.size());
        Pair p = heap_array.front();
        indices_map[p.first] = POST_HEAP;
        if(n > 1)
            adjust_heap(Difference(sizeof(Pair)), n * sizeof(Pair),
                        std::move(heap_array.back()));
        heap_array.pop_back();
        return p;
    }
    void decrease(const Node & u, const Prio & p) noexcept {
        heap_push(indices_map[u], Pair(u, p));
    }
    State state(const Node & u) const noexcept {
        return State(std::min(indices_map[u], Difference(sizeof(Pair))));
    }
};  // class BinHeap

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_BINARY_HEAP_HPP

#ifndef MELON_DIJKSTRA_SEMIRINGS_HPP
#define MELON_DIJKSTRA_SEMIRINGS_HPP

#include <functional>

namespace fhamonic {
namespace melon {

template <typename T>
struct DijkstraShortestPathSemiring {
    static constexpr T zero = static_cast<T>(0);
    static constexpr std::plus<T> plus{};
    static constexpr std::less<T> less{};
};

template <typename T>
struct DijkstraMostProbablePathSemiring {
    static constexpr T zero = static_cast<T>(1);
    static constexpr std::multiplies<T> plus{};
    static constexpr std::greater<T> less{};
};

template <typename T>
struct DijkstraMaxFlowPathSemiring {
    static constexpr T zero = std::numeric_limits<T>::max();
    static constexpr auto plus = [](const T & a, const T & b) {
        return std::min(a, b);
    };
    static constexpr std::greater<T> less{};
};

template <typename T>
struct DijkstraSpanningTreeSemiring {
    static constexpr T zero = static_cast<T>(0);
    static constexpr auto plus = [](const T & a, const T & b) { return b; };
    static constexpr std::less<T> less{};
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_DIJKSTRA_SEMIRINGS_HPP

namespace fhamonic {
namespace melon {

template <typename GR, typename LM,
          typename SR = DijkstraShortestPathSemiring<typename LM::value_type>,
          typename HP = BinaryHeap<typename GR::Node, typename LM::value_type,
                                   decltype(SR::less)>>
class Dijkstra {
public:
    using Node = GR::Node;
    using Arc = GR::Arc;

    using Value = LM::value_type;
    using DijkstraSemiringTraits = SR;
    using Heap = HP;

private:
    const GR & graph;
    const LM & length_map;

    Heap heap;
    typename GR::ArcMap<Node> pred_map;

public:
    Dijkstra(const GR & g, const LM & l)
        : graph(g), length_map(l), heap(g.nb_nodes()), pred_map(g.nb_nodes()) {}

    void addSource(Node s, Value dist = DijkstraSemiringTraits::zero) noexcept {
        assert(!heap.contains(s));
        heap.push(s, dist);
        pred_map[s] = s;
    }
    bool emptyQueue() const noexcept { return heap.empty(); }
    void reset() noexcept { heap.clear(); }

    std::pair<Node, Value> processNextNode() noexcept {
        const auto p = heap.pop();
        for(Arc a : graph.out_arcs(p.first)) {
            Node w = graph.target(a);
            const auto s = heap.state(w);
            if(s == Heap::IN_HEAP) [[likely]] {
                Value new_dist =
                    DijkstraSemiringTraits::plus(p.second, length_map[a]);
                if(DijkstraSemiringTraits::less(new_dist, heap.prio(w))) {
                    heap.decrease(w, new_dist);
                    pred_map[w] = p.first;
                }
                continue;
            }
            if(s == Heap::PRE_HEAP) {
                heap.push(
                    w, DijkstraSemiringTraits::plus(p.second, length_map[a]));
                pred_map[w] = p.first;
            }
        }
        return p;
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_DIJKSTRA_HPP

#endif  // FHAMONIC_MELON_HPP
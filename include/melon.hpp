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
#include <ranges>
#include <vector>

#ifndef MELON_STATIC_MAP_HPP
#define MELON_STATIC_MAP_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <vector>

namespace fhamonic {
namespace melon {

template <typename T>
class StaticMap {
public:
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using iterator = T *;
    using const_iterator = const T *;
    using size_type = std::size_t;

private:
    std::unique_ptr<value_type[]> _data;
    value_type * _data_end;

public:
    StaticMap() : _data(nullptr), _data_end(nullptr){};
    StaticMap(std::size_t size)
        : _data(std::make_unique<value_type[]>(size))
        , _data_end(_data.get() + size){};

    StaticMap(std::size_t n, value_type init_value) : StaticMap(n) {
        std::ranges::fill(*this, init_value);
    };

    StaticMap(const StaticMap & other) : StaticMap(other.size()) {
        std::ranges::copy(other, _data);
    };
    StaticMap(StaticMap &&) = default;

    StaticMap & operator=(const StaticMap & other) {
        resize(other.size());
        std::ranges::copy(other, _data);
    };
    StaticMap & operator=(StaticMap &&) = default;

    iterator begin() noexcept { return _data.get(); }
    iterator end() noexcept { return _data_end; }
    const_iterator cbegin() const noexcept { return _data.get(); }
    const_iterator cend() const noexcept { return _data_end; }

    size_type size() const noexcept { return std::distance(cbegin(), cend()); }
    void resize(size_type n) {
        if(n == size()) return;
        _data = std::make_unique<value_type[]>(n);
        _data_end = _data.get() + n;
    }

    reference operator[](size_type i) noexcept { return _data[i]; }
    const_reference operator[](size_type i) const noexcept { return _data[i]; }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_STATIC_MAP_HPP

namespace fhamonic {
namespace melon {

class StaticDigraph {
public:
    using Node = unsigned int;
    using Arc = unsigned int;

    template <typename T>
    using NodeMap = StaticMap<T>;
    template <typename T>
    using ArcMap = StaticMap<T>;

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

#ifndef MELON_BFS_HPP
#define MELON_BFS_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <type_traits>  // underlying_type, conditional
#include <variant>      // monostate
#include <vector>

#ifndef MELON_NODE_SEARCH_BEHAVIOR
#define MELON_NODE_SEARCH_BEHAVIOR

namespace fhamonic {
namespace melon {

enum NodeSeachBehavior : unsigned char {
    TRACK_NONE = 0b00000000,
    TRACK_PRED_NODES = 0b00000001,
    TRACK_PRED_ARCS = 0b00000010,
    TRACK_DISTANCES = 0b00000100
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_NODE_SEARCH_BEHAVIOR

namespace fhamonic {
namespace melon {

template <typename GR, std::underlying_type_t<NodeSeachBehavior> BH =
                           NodeSeachBehavior::TRACK_NONE>
class BFS {
public:
    using Node = GR::Node;
    using Arc = GR::Arc;

    using ReachedMap = typename GR::NodeMap<bool>;

    static constexpr bool track_predecessor_nodes =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_PRED_NODES);
    static constexpr bool track_predecessor_arcs =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_PRED_ARCS);
    static constexpr bool track_distances =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_DISTANCES);

    using PredNodesMap =
        std::conditional<track_predecessor_nodes, typename GR::NodeMap<Node>,
                         std::monostate>::type;
    using PredArcsMap =
        std::conditional<track_predecessor_arcs, typename GR::NodeMap<Arc>,
                         std::monostate>::type;
    using DistancesMap =
        std::conditional<track_distances, typename GR::NodeMap<std::size_t>,
                         std::monostate>::type;

private:
    const GR & graph;
    std::vector<Node> queue;
    std::vector<Node>::iterator queue_current;

    ReachedMap reached_map;

    PredNodesMap pred_nodes_map;
    PredArcsMap pred_arcs_map;
    DistancesMap dist_map;

public:
    BFS(const GR & g) : graph(g), queue(), reached_map(g.nb_nodes(), false) {
        queue.reserve(g.nb_nodes());
        queue_current = queue.begin();
        if constexpr(track_predecessor_nodes)
            pred_nodes_map.resize(g.nb_nodes());
        if constexpr(track_predecessor_arcs) pred_arcs_map.resize(g.nb_nodes());
        if constexpr(track_distances) dist_map.resize(g.nb_nodes());
    }

    BFS & reset() noexcept {
        queue.resize(0);
        std::ranges::fill(reached_map, false);
        return *this;
    }
    BFS & addSource(Node s) noexcept {
        assert(!reached_map[s]);
        pushNode(s);
        if constexpr(track_predecessor_nodes) pred_nodes_map[s] = s;
        if constexpr(track_distances) dist_map[s] = 0;
        return *this;
    }

    bool emptyQueue() const noexcept { return queue_current == queue.end(); }
    void pushNode(Node u) noexcept {
        queue.push_back(u);
        reached_map[u] = true;
    }
    Node popNode() noexcept {
        Node u = *queue_current;
        ++queue_current;
        return u;
    }
    bool reached(const Node u) const noexcept { return reached_map[u]; }

    Node processNextNode() noexcept {
        const Node u = popNode();
        for(Arc a : graph.out_arcs(u)) {
            Node w = graph.target(a);
            if(reached_map[w]) continue;
            pushNode(w);
            if constexpr(track_predecessor_nodes) pred_nodes_map[w] = u;
            if constexpr(track_predecessor_arcs) pred_arcs_map[w] = a;
            if constexpr(track_distances) dist_map[w] = dist_map[u] + 1;
        }
        return u;
    }

    void run() noexcept {
        while(!emptyQueue()) {
            processNextNode();
        }
    }

    Node pred_node(const Node u) const noexcept
        requires(track_predecessor_nodes) {
        assert(reached(u));
        return pred_nodes_map[u];
    }
    Arc pred_arc(const Node u) const noexcept requires(track_predecessor_arcs) {
        assert(reached(u));
        return pred_arcs_map[u];
    }
    std::size_t dist(const Node u) const noexcept requires(track_distances) {
        assert(reached(u));
        return dist_map[u];
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_BFS_HPP

#ifndef MELON_DFS_HPP
#define MELON_DFS_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <stack>
#include <type_traits>  // underlying_type, conditional
#include <variant>      // monostate
#include <vector>

namespace fhamonic {
namespace melon {

// TODO ranges , requires out_arcs : borrowed_range
template <typename GR, std::underlying_type_t<NodeSeachBehavior> BH =
                           NodeSeachBehavior::TRACK_NONE>
class DFS {
public:
    using Node = GR::Node;
    using Arc = GR::Arc;

    using ReachedMap = typename GR::NodeMap<bool>;

    static constexpr bool track_predecessor_nodes =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_PRED_NODES);
    static constexpr bool track_predecessor_arcs =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_PRED_ARCS);

    using PredNodesMap =
        std::conditional<track_predecessor_nodes, typename GR::NodeMap<Node>,
                         std::monostate>::type;
    using PredArcsMap =
        std::conditional<track_predecessor_arcs, typename GR::NodeMap<Arc>,
                         std::monostate>::type;

private:
    const GR & graph;

    using OutArcRange = decltype(std::declval<GR>().out_arcs(Node()));
    using OutArcIt = decltype(std::declval<OutArcRange>().begin());
    using OutArcItEnd = decltype(std::declval<OutArcRange>().end());

    static_assert(std::ranges::borrowed_range<OutArcRange>);
    std::vector<std::pair<OutArcIt, OutArcItEnd>> stack;

    ReachedMap reached_map;

    PredNodesMap pred_nodes_map;
    PredArcsMap pred_arcs_map;

public:
    DFS(const GR & g) : graph(g), stack(), reached_map(g.nb_nodes(), false) {
        stack.reserve(g.nb_nodes());
        if constexpr(track_predecessor_nodes)
            pred_nodes_map.resize(g.nb_nodes());
        if constexpr(track_predecessor_arcs) pred_arcs_map.resize(g.nb_nodes());
    }

    DFS & reset() noexcept {
        stack.resize(0);
        std::ranges::fill(reached_map, false);
        return *this;
    }
    DFS & addSource(Node s) noexcept {
        assert(!reached_map[s]);
        pushNode(s);
        if constexpr(track_predecessor_nodes) pred_nodes_map[s] = s;
        return *this;
    }

    bool emptyQueue() const noexcept { return stack.empty(); }
    void pushNode(Node u) noexcept {
        OutArcRange r = graph.out_arcs(u);
        stack.emplace_back(r.begin(), r.end());
        reached_map[u] = true;
    }
    bool reached(const Node u) const noexcept { return reached_map[u]; }

private:
    void advance_iterators() {
        assert(!stack.empty());
        do {
            while(stack.back().first != stack.back().second) {
                if(!reached(graph.target(*stack.back().first))) return;
                ++stack.back().first;
            }
            stack.pop_back();
        } while(!stack.empty());
    }

public:
    std::pair<Arc, Node> processNextNode() noexcept {
        Arc a = *stack.back().first;
        Node u = graph.target(a);
        pushNode(u);
        // if constexpr(track_predecessor_nodes) pred_nodes_map[u] = u;
        if constexpr(track_predecessor_arcs) pred_arcs_map[u] = a;
        advance_iterators();
        return std::make_pair(a, u);
    }

    void run() noexcept {
        while(!emptyQueue()) {
            processNextNode();
        }
    }

    Node pred_node(const Node u) const noexcept
        requires(track_predecessor_nodes) {
        assert(reached(u));
        return pred_nodes_map[u];
    }
    Arc pred_arc(const Node u) const noexcept requires(track_predecessor_arcs) {
        assert(reached(u));
        return pred_arcs_map[u];
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_DFS_HPP

#ifndef MELON_DIJKSTRA_HPP
#define MELON_DIJKSTRA_HPP

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <variant>
#include <vector>

#ifndef MELON_D_ARY_HEAP_HPP
#define MELON_D_ARY_HEAP_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace fhamonic {
namespace melon {

template <int D, typename ND, typename PR, typename CMP = std::less<PR>>
class DAryHeap {
public:
    using Node = ND;
    using Prio = PR;
    using Compare = CMP;
    using Pair = std::pair<Node, Prio>;

private:
    using Index = std::vector<Pair>::size_type;

public:
    enum State : char { PRE_HEAP = 0, IN_HEAP = 1, POST_HEAP = 2 };

    std::vector<Pair> heap_array;
    std::vector<Index> indices_map;
    std::vector<State> states_map;
    Compare cmp;

public:
    DAryHeap(const std::size_t nb_nodes)
        : heap_array()
        , indices_map(nb_nodes)
        , states_map(nb_nodes, State::PRE_HEAP)
        , cmp() {}

    DAryHeap(const DAryHeap & bin) = default;
    DAryHeap(DAryHeap && bin) = default;

    Index size() const noexcept { return heap_array.size(); }
    bool empty() const noexcept { return heap_array.empty(); }
    void clear() noexcept {
        heap_array.resize(0);
        std::ranges::fill(states_map, State::PRE_HEAP);
    }

private:
    static constexpr Index parent_of(Index i) {
        return (i - sizeof(Pair)) / (sizeof(Pair) * D) * sizeof(Pair);
    }
    static constexpr Index first_child_of(Index i) {
        return i * D + sizeof(Pair);
    }
    template <int I = D>
    constexpr Index minimum_child(const Index first_child) const {
        if constexpr(I == 1)
            return first_child;
        else if constexpr(I == 2)
            return first_child +
                   sizeof(Pair) *
                       cmp(pair_ref(first_child + sizeof(Pair)).second,
                           pair_ref(first_child).second);
        else {
            const Index first_half_minimum = minimum_child<I / 2>(first_child);
            const Index second_half_minimum =
                minimum_child<I - I / 2>(first_child + (I / 2) * sizeof(Pair));
            return cmp(pair_ref(second_half_minimum).second,
                       pair_ref(first_half_minimum).second)
                       ? second_half_minimum
                       : first_half_minimum;
        }
    }
    constexpr Index minimum_remaining_child(const Index first_child,
                                            const Index nb_children) const {
        if constexpr(D == 2)
            return first_child;
        else if constexpr(D == 4) {
            switch(nb_children) {
                case 1:
                    return minimum_child<1>(first_child);
                case 2:
                    return minimum_child<2>(first_child);
                default:
                    return minimum_child<3>(first_child);
            }
        } else {
            switch(nb_children) {
                case 1:
                    return minimum_child<1>(first_child);
                case 2:
                    return minimum_child<2>(first_child);
                default:
                    const Index half = nb_children / 2;
                    const Index first_half_minimum =
                        minimum_remaining_child(first_child, half);
                    const Index second_half_minimum = minimum_remaining_child(
                        first_child + half * sizeof(Pair), nb_children - half);
                    return cmp(pair_ref(second_half_minimum).second,
                               pair_ref(first_half_minimum).second)
                               ? second_half_minimum
                               : first_half_minimum;
            }
        }
    }

    constexpr Pair & pair_ref(Index i) {
        assert(0 <= (i / sizeof(Pair)) &&
               (i / sizeof(Pair)) < heap_array.size());
        return *(reinterpret_cast<Pair *>(
            reinterpret_cast<std::byte *>(heap_array.data()) + i));
    }
    constexpr const Pair & pair_ref(Index i) const {
        assert(0 <= (i / sizeof(Pair)) &&
               (i / sizeof(Pair)) < heap_array.size());
        return *(reinterpret_cast<const Pair *>(
            reinterpret_cast<const std::byte *>(heap_array.data()) + i));
    }
    void heap_move(Index i, Pair && p) noexcept {
        assert(0 <= (i / sizeof(Pair)) &&
               (i / sizeof(Pair)) < heap_array.size());
        indices_map[p.first] = i;
        pair_ref(i) = std::move(p);
    }

    void heap_push(Index holeIndex, Pair && p) noexcept {
        while(holeIndex > 0) {
            Index parent = parent_of(holeIndex);
            if(!cmp(p.second, pair_ref(parent).second)) break;
            heap_move(holeIndex, std::move(pair_ref(parent)));
            holeIndex = parent;
        }
        heap_move(holeIndex, std::move(p));
    }

    void adjust_heap(Index holeIndex, const Index end, Pair && p) noexcept {
        Index child_end;
        if constexpr(D > 2)
            child_end =
                end > D * sizeof(Pair) ? end - (D - 1) * sizeof(Pair) : 0;
        else
            child_end = end - (D - 1) * sizeof(Pair);
        Index child = first_child_of(holeIndex);
        while(child < child_end) {
            child = minimum_child(child);
            if(cmp(pair_ref(child).second, p.second)) {
                heap_move(holeIndex, std::move(pair_ref(child)));
                holeIndex = child;
                child = first_child_of(child);
                continue;
            }
            goto ok;
        }
        if(child < end) {
            child =
                minimum_remaining_child(child, (end - child) / sizeof(Pair));
            if(cmp(pair_ref(child).second, p.second)) {
                heap_move(holeIndex, std::move(pair_ref(child)));
                holeIndex = child;
            }
        }
    ok:
        heap_move(holeIndex, std::move(p));
    }

public:
    void push(Pair && p) noexcept {
        const Index n = heap_array.size();
        heap_array.emplace_back();
        states_map[p.first] = IN_HEAP;
        heap_push(Index(n * sizeof(Pair)), std::move(p));
    }
    void push(const Node i, const Prio p) noexcept { push(Pair(i, p)); }
    Prio prio(const Node u) const noexcept {
        return pair_ref(indices_map[u]).second;
    }
    Pair top() const noexcept {
        assert(!heap_array.empty());
        return heap_array.front();
    }
    Pair pop() noexcept {
        assert(!heap_array.empty());
        const Pair p = std::move(heap_array.front());
        states_map[p.first] = POST_HEAP;
        const Index n = heap_array.size() - 1;
        if(n > 0)
            adjust_heap(Index(0), n * sizeof(Pair),
                        std::move(heap_array.back()));
        heap_array.pop_back();
        return p;
    }
    void decrease(const Node & u, const Prio & p) noexcept {
        heap_push(indices_map[u], Pair(u, p));
    }
    State state(const Node & u) const noexcept { return states_map[u]; }
};  // class DAryHeap

template <typename ND, typename PR, typename CMP = std::less<PR>>
using BinaryHeap = DAryHeap<2, ND, PR, CMP>;

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_D_ARY_HEAP_HPP
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

#ifndef MELON_FAST_BINARY_HEAP_HPP
#define MELON_FAST_BINARY_HEAP_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

namespace fhamonic {
namespace melon {

template <typename ND, typename PR, typename CMP = std::less<PR>>
class FastBinaryHeap {
public:
    using Node = ND;
    using Prio = PR;
    using Compare = CMP;
    using Pair = std::pair<Node, Prio>;

private:
    using Index = std::vector<Pair>::size_type;

public:
    static_assert(sizeof(Pair) >= 2, "std::pair<Node, Prio> is too small");
    enum State : Index {
        PRE_HEAP = static_cast<Index>(0),
        POST_HEAP = static_cast<Index>(1),
        IN_HEAP = static_cast<Index>(2)
    };

    std::vector<Pair> heap_array;
    std::vector<Index> indices_map;
    Compare cmp;

public:
    FastBinaryHeap(const std::size_t nb_nodes)
        : heap_array(1), indices_map(nb_nodes, State::PRE_HEAP), cmp() {}

    FastBinaryHeap(const FastBinaryHeap & bin) = default;
    FastBinaryHeap(FastBinaryHeap && bin) = default;

    Index size() const noexcept { return heap_array.size() - 1; }
    bool empty() const noexcept { return size() == 0; }
    void clear() noexcept {
        heap_array.resize(1);
        std::ranges::fill(indices_map, State::PRE_HEAP);
    }

private:
    constexpr Pair & pair_ref(Index i) {
        return *(reinterpret_cast<Pair *>(
            reinterpret_cast<std::byte *>(heap_array.data()) + i));
    }
    constexpr const Pair & pair_ref(Index i) const {
        return *(reinterpret_cast<const Pair *>(
            reinterpret_cast<const std::byte *>(heap_array.data()) + i));
    }

    void heap_move(Index index, Pair && p) noexcept {
        indices_map[p.first] = index;
        pair_ref(index) = std::move(p);
    }

    void heap_push(Index holeIndex, Pair && p) noexcept {
        while(holeIndex > sizeof(Pair)) {
            Index parent = holeIndex / (2 * sizeof(Pair)) * sizeof(Pair);
            if(!cmp(p.second, pair_ref(parent).second)) break;
            heap_move(holeIndex, std::move(pair_ref(parent)));
            holeIndex = parent;
        }
        heap_move(holeIndex, std::move(p));
    }

    void adjust_heap(Index holeIndex, const Index end, Pair && p) noexcept {
        Index child = 2 * holeIndex;
        while(child < end) {
            child += sizeof(Pair) * cmp(pair_ref(child + sizeof(Pair)).second,
                                        pair_ref(child).second);
            if(cmp(pair_ref(child).second, p.second)) {
                heap_move(holeIndex, std::move(pair_ref(child)));
                holeIndex = child;
                child = 2 * holeIndex;
                continue;
            }
            goto ok;
        }
        if(child == end && cmp(pair_ref(child).second, p.second)) {
            heap_move(holeIndex, std::move(pair_ref(child)));
            holeIndex = child;
        }
    ok:
        heap_move(holeIndex, std::move(p));
    }

public:
    void push(Pair && p) noexcept {
        heap_array.emplace_back();
        heap_push(static_cast<Index>(size() * sizeof(Pair)), std::move(p));
    }
    void push(const Node i, const Prio p) noexcept { push(Pair(i, p)); }
    bool contains(const Node u) const noexcept { return indices_map[u] > 0; }
    Prio prio(const Node u) const noexcept {
        return pair_ref(indices_map[u]).second;
    }
    Pair top() const noexcept {
        assert(!empty());
        return heap_array[1];
    }
    Pair pop() noexcept {
        assert(!empty());
        const Index n = heap_array.size() - 1;
        const Pair p = std::move(heap_array[1]);
        indices_map[p.first] = POST_HEAP;
        if(n > 1)
            adjust_heap(static_cast<Index>(sizeof(Pair)), n * sizeof(Pair),
                        std::move(heap_array.back()));
        heap_array.pop_back();
        return p;
    }
    void decrease(const Node & u, const Prio & p) noexcept {
        heap_push(indices_map[u], Pair(u, p));
    }
    State state(const Node & u) const noexcept {
        return State(std::min(indices_map[u], static_cast<Index>(IN_HEAP)));
    }
};  // class FastBinaryHeap

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_FAST_BINARY_HEAP_HPP

namespace fhamonic {
namespace melon {

template <typename GR, typename LM,
          std::underlying_type_t<NodeSeachBehavior> BH =
              NodeSeachBehavior::TRACK_NONE,
          typename SR = DijkstraShortestPathSemiring<typename LM::value_type>,
          typename HP = FastBinaryHeap<
              typename GR::Node, typename LM::value_type, decltype(SR::less)>>
class Dijkstra {
public:
    using Node = GR::Node;
    using Arc = GR::Arc;

    using Value = LM::value_type;
    using DijkstraSemiringTraits = SR;
    using Heap = HP;

    static constexpr bool track_predecessor_nodes =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_PRED_NODES);
    static constexpr bool track_predecessor_arcs =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_PRED_ARCS);
    static constexpr bool track_distances =
        static_cast<bool>(BH & NodeSeachBehavior::TRACK_DISTANCES);

    using PredNodesMap =
        std::conditional<track_predecessor_nodes, typename GR::NodeMap<Node>,
                         std::monostate>::type;
    using PredArcsMap =
        std::conditional<track_predecessor_arcs, typename GR::NodeMap<Arc>,
                         std::monostate>::type;
    using DistancesMap =
        std::conditional<track_distances, typename GR::NodeMap<Value>,
                         std::monostate>::type;

private:
    const GR & graph;
    const LM & length_map;

    Heap heap;
    PredNodesMap pred_nodes_map;
    PredArcsMap pred_arcs_map;
    DistancesMap dist_map;

public:
    Dijkstra(const GR & g, const LM & l)
        : graph(g), length_map(l), heap(g.nb_nodes()) {
        if constexpr(track_predecessor_nodes)
            pred_nodes_map.resize(g.nb_nodes());
        if constexpr(track_predecessor_arcs) pred_arcs_map.resize(g.nb_nodes());
        if constexpr(track_distances) dist_map.resize(g.nb_nodes());
    }

    Dijkstra & reset() noexcept {
        heap.clear();
        return *this;
    }
    Dijkstra & addSource(Node s,
                         Value dist = DijkstraSemiringTraits::zero) noexcept {
        assert(heap.state(s) != Heap::IN_HEAP);
        heap.push(s, dist);
        if constexpr(track_predecessor_nodes) pred_nodes_map[s] = s;
        return *this;
    }

    bool emptyQueue() const noexcept { return heap.empty(); }

    std::pair<Node, Value> processNextNode() noexcept {
        const auto p = heap.pop();
        for(Arc a : graph.out_arcs(p.first)) {
            Node w = graph.target(a);
            const auto s = heap.state(w);
            if(s == Heap::IN_HEAP) {
                Value new_dist =
                    DijkstraSemiringTraits::plus(p.second, length_map[a]);
                if(DijkstraSemiringTraits::less(new_dist, heap.prio(w))) {
                    heap.decrease(w, new_dist);
                    if constexpr(track_predecessor_nodes)
                        pred_nodes_map[w] = p.first;
                    if constexpr(track_predecessor_arcs) pred_arcs_map[w] = a;
                }
                continue;
            }
            if(s == Heap::PRE_HEAP) {
                heap.push(
                    w, DijkstraSemiringTraits::plus(p.second, length_map[a]));
                if constexpr(track_predecessor_nodes)
                    pred_nodes_map[w] = p.first;
                if constexpr(track_predecessor_arcs) pred_arcs_map[w] = a;
            }
        }
        if constexpr(track_distances) dist_map[p.first] = p.second;
        return p;
    }

    void run() noexcept {
        while(!emptyQueue()) {
            processNextNode();
        }
    }

    Node pred_node(const Node u) const noexcept
        requires(track_predecessor_nodes) {
        assert(heap.state(u) != Heap::PRE_HEAP);
        return pred_nodes_map[u];
    }
    Arc pred_arc(const Node u) const noexcept requires(track_predecessor_arcs) {
        assert(heap.state(u) != Heap::PRE_HEAP);
        return pred_arcs_map[u];
    }
    Value dist(const Node u) const noexcept requires(track_distances) {
        assert(heap.state(u) == Heap::POST_HEAP);
        return dist_map[u];
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_DIJKSTRA_HPP

#ifndef MELON_NODE_SEARCH_SPAN_HPP
#define MELON_NODE_SEARCH_SPAN_HPP

#include <concepts>
#include <iterator>

namespace fhamonic {
namespace melon {

template <typename Algo>
concept node_search_algorithm = requires(Algo alg) {
    { alg.emptyQueue() }
    ->std::convertible_to<bool>;
    { alg.processNextNode() }
    ->std::default_initializable;
};

template <typename Algo>
requires node_search_algorithm<Algo> struct node_search_span {
    struct end_iterator {};
    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = decltype(std::declval<Algo>().processNextNode());
        using reference = value_type const &;
        using pointer = value_type *;

        iterator(Algo & alg) : algorithm(alg) {}
        iterator & operator++() noexcept {
            node = algorithm.processNextNode();
            return *this;
        }
        friend bool operator==(const iterator & it, end_iterator) noexcept {
            return it.algorithm.emptyQueue();
        }
        reference operator*() const noexcept { return node; }

    private:
        Algo & algorithm;

    public:
        value_type node;
    };

    iterator begin() {
        iterator it{algorithm};
        if(!algorithm.emptyQueue()) ++it;
        return it;
    }
    end_iterator end() noexcept { return {}; }

    node_search_span(node_search_span const &) = delete;

public:
    explicit node_search_span(Algo & alg) : algorithm(alg) {}
    Algo & algorithm;
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_NODE_SEARCH_SPAN_HPP

#endif  // FHAMONIC_MELON_HPP
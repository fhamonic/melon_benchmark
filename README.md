# MELON benchmark

Runtime comparison of [MELON](https://github.com/fhamonic/melon) against other C++
graph libraries — [LEMON](https://lemon.cs.elte.hu/) and
[Boost.Graph](https://www.boost.org/doc/libs/release/libs/graph/) on graph algorithms,
[CGAL](https://www.cgal.org/) on the Bentley–Ottmann segment-intersection sweep.

Interactive charts of every recorded run: **https://fhamonic.github.io/melon_benchmark/**

All numbers below are medians of 5 repetitions on an AMD Ryzen 7 7800X3D, GCC 14.1.0,
`-O3` + LTO, generic x86-64 codegen. Every library is built with the **same compiler**
(only the language standard differs: MELON requires C++23, the baselines build as C++20),
and every timed configuration must produce the same result digest as all the others — see
[Validation](#validation) — so nothing here wins by computing less. The raw Google
Benchmark JSON behind every table is tracked under `results/`.

## Results

### Throughput

`USA-road-t.NE` (1.52 M vertices, 3.9 M arcs), each library in its fastest graph container:

| benchmark | MELON | LEMON | Boost |
|---|---:|---:|---:|
| `breadth_first_search` | **9.7 ms** | 31.7 ms (3.3×) | 26.2 ms (2.7×) |
| `depth_first_search` | **20.5 ms** | 35.6 ms (1.7×) | 46.6 ms (2.3×) |
| `dijkstra` `int/4-heap` | **74.3 ms** | 115.8 ms (1.6×) | 143.1 ms (1.9×) |
| `strongly_connected_components` | **29.6 ms** | 75.7 ms (2.6×) | 83.9 ms (2.8×) |
| `traversal_forest` | **10.5 ms** | 27.7 ms (2.6×) | 62.6 ms (6.0×) |
| `weakly_connected_components` | **32.6 ms** | 63.1 ms (1.9×) | — (none correct) |

Boost has no correct weak-components algorithm for digraphs: `boost::connected_components`
(and `lemon::connectedComponents`) on a directed graph actually compute the partition induced
by repeated traversal from unvisited vertices. MELON exposes that operation under its honest
name, `traversal_forest`, and that chart compares equivalent work across all three libraries.

### Point-to-point: bidirectional search

`dijkstra_point_to_point` asks each library for the distance between one source and one
target, over the same deterministic source/target pairs. MELON is the only one of the three
with a `bidirectional_dijkstra`, which grows a search from each end and stops when they meet.
Per query, on MELON's `static_digraph`:

| instance | \|V\| | unidirectional | bidirectional | speedup |
|---|---:|---:|---:|---:|
| `rome99` | 3.4 K | 71.1 µs | 67.1 µs | 1.06× |
| `USA-road-t.NY` | 264 K | 7.06 ms | 4.64 ms | 1.52× |
| `USA-road-t.BAY` | 321 K | 8.44 ms | 6.49 ms | 1.30× |
| `USA-road-t.COL` | 436 K | 6.90 ms | 4.09 ms | 1.69× |
| `USA-road-t.FLA` | 1.07 M | 27.59 ms | 36.24 ms | 0.76× |
| `USA-road-t.NW` | 1.21 M | 19.34 ms | 13.66 ms | 1.42× |
| `USA-road-t.NE` | 1.52 M | 60.43 ms | **35.69 ms** | 1.69× |

Bidirectional search wins by exploring less area, and typically saves 1.3–1.7× here. It is
not a guaranteed win — on `FLA` the two frontiers meet late for these query pairs and the
double bookkeeping costs more than it saves — which is why the whole table is published.
For reference, the fastest baseline on `USA-road-t.NE` (LEMON `StaticDigraph`) answers the
same query unidirectionally in 97.5 ms.

### Short searches: what MELON's traits buy

MELON's Dijkstra can be told at compile time not to build the distance map or the
shortest-path tree (`store_distances` / `store_paths`). Because Dijkstra's per-query setup
is O(|V|) however far the search runs, that matters enormously for queries that stop early.
The `dijkstra_bounded` benchmark is a k-nearest-vertices query — consume each distance as
the vertex is settled, then stop — with every library in the leanest configuration its own
API allows (`USA-road-t.NE`, µs **per query**):

| k | MELON streaming | MELON stored | LEMON streaming | Boost streaming |
|---:|---:|---:|---:|---:|
| 100 | **14 µs** | 200 µs (14×) | 321 µs (23×) | 1 066 µs (75×) |
| 1 000 | **25 µs** | 216 µs (8.7×) | 338 µs (14×) | 1 093 µs (44×) |
| 10 000 | **338 µs** | 587 µs (1.7×) | 844 µs (2.5×) | 1 617 µs (4.8×) |
| unbounded | **69.0 ms** | 73.7 ms (1.1×) | 110.7 ms (1.6×) | 147.7 ms (2.1×) |

For short searches on large graphs — service areas, k-nearest, any query that consumes
distances online and stops — a MELON query costs one to two orders of magnitude less than
the same query written against LEMON's or Boost's APIs. As the search grows to cover the
graph the advantage amortizes away to ~10% over MELON's own storing mode.

**Read the short-k rows as per-query setup cost, not inner-loop speed.** At k = 100 most of
the measured time is constructing the algorithm object: maps a library cannot avoid
allocating and initializing for all |V| vertices. Isolating the two traits shows
`store_paths` carries essentially the whole cost (a value-initialized predecessor map) and
`store_distances` costs nothing measurable — so if you need distances but not paths,
`store_paths = false` alone gets you the whole win. The baselines are configured as leanly
as their APIs permit: LEMON gets `NullMap` for its predecessor, distance and processed maps;
Boost cannot drop its distance map because its d-ary heap is keyed on it.

### Bentley–Ottmann vs CGAL

MELON's plane-sweep segment intersection is compared against CGAL's exact-kernel (`epeck`)
surface sweep on seeded random segment families, n up to 1024, with intersection-dense
(`box`) and sparse (`vec`) geometry. Every MELON coordinate type must produce the same
event-count digest as CGAL. On the largest instances MELON is **2.2–3.4× faster** than
CGAL depending on coordinate type (e.g. `box_n1024`, 8-bit coordinates: 38.7 ms with
`int64` vs CGAL's 129.8 ms), while remaining exact on its bounded integer types.

## Validation

Every benchmark computes its result once, outside the timed region, reduces it to a 64-bit
digest and publishes it as the run's label. `validate.py` asserts that **every library,
every graph container and every value type produced the same digest** per (algorithm,
dataset, instance); `make plot` refuses to draw anything otherwise. Digests are canonical:
values are read in vertex-id order (never traversal order), component partitions are
canonicalized before hashing, and values are quantized so the `int` series of an algorithm
checks against its `double` series. Max-flow and min-cost-flow results are additionally
checked against shipped optimal `.sol` values.

Three benchmarks need a note. `network_simplex` digests the optimal *cost* rather than the
flow: degenerate optima let two correct implementations return different flows of the same
cost, so the flow is not a canonical answer and the cost is. `traversal_forest`'s result
legitimately depends on the container's root-iteration order, so `validate.py` checks
agreement within each root order instead of across all. And `kruskal` currently has a known
int-vs-double digest mismatch on the four largest road networks (all libraries agree within
each value type — it is an equal-weight tie-breaking artifact, not a wrong tree), so no
kruskal timings are published above.

Timed loops are guarded against elision: `benchmark::DoNotOptimize` on every produced
value, and `dijkstra_bounded` counts settled vertices *inside* the timed loop and requires
the count to read exactly `min(k, reachable)` for every library.

## What is measured

Parsing and graph construction happen **outside** the timed region. Single-source
algorithms run once per source over an identical, deterministic source set sized to a
constant complexity budget; the algorithm object is constructed inside the loop, so
per-query setup is included — deliberately, it is part of the cost of answering a query.
Except where a leaner configuration is the point (`dijkstra_bounded`), all libraries do
the same bookkeeping: for Dijkstra, a distance map and a predecessor map per query,
distances consumed as vertices are settled. MELON is faster still with its storage traits
off, but that is measured as its own benchmark, not smuggled into the comparison.

## Coverage

| Algorithm | Dataset | MELON | LEMON | Boost |
|---|---|---|---|---|
| `dijkstra` | 9th_dimacs | static/mutable × {int,double} × {2,4,8}-heap | Static/Smart/List × {int,double} × {2,4,8}-heap | adjacency_list + CSR × {int,double}, 4-ary heap only |
| `dijkstra_point_to_point` | 9th_dimacs | {unidirectional, **bidirectional**} | ✅ | ✅ |
| `dijkstra_bounded` | 9th_dimacs | {stored, streaming} × k | {stored, streaming} × k | {stored, streaming} × k |
| `breadth_first_search` | snap, 9th_dimacs | ✅ | ✅ | ✅ |
| `depth_first_search` | snap, 9th_dimacs | ✅ | ✅ | ✅ |
| `strongly_connected_components` | snap, 9th_dimacs | ✅ | ✅ | CSR only |
| `weakly_connected_components` | snap, 9th_dimacs | ✅ | ✅ | ❌ none correct |
| `traversal_forest` | snap, 9th_dimacs | ✅ | ✅ | CSR only |
| `max_flow` | bvz_tsukuba, rmf, snap | dinitz | preflow | push_relabel, boykov_kolmogorov (not snap) |
| `network_simplex` | netgen, assignment, transport, circulation | static/mutable × {int,double} | Static/Smart/List × {int,double} | ❌ none |
| `kruskal` | 9th_dimacs, bvz_tsukuba | ✅ | ✅ | ❌ |
| `bentley_ottmann` | random segments | int64/int128/bounded8/16/32 | vs **CGAL** `epeck` | — |

The `max_flow` chart compares each library's flagship algorithm, so it answers "what does
each library give you", not "whose Dinitz is faster". Boost's Boykov–Kolmogorov is included
because BVZ-tsukuba is the vision dataset it was published for — and on it, it is roughly an
order of magnitude faster than everything else.

The `network_simplex` chart is the opposite kind of comparison: MELON's implementation is a
reimplementation of LEMON's, both are passed the block-search pivot rule explicitly with the
same block-size constants, both search the same arc set (supplies balance, so LEMON takes its
EQ branch and its artificial arcs stay out of the search, matching MELON's virtual arcs), and
both are handed the same capacities, costs and supplies — so it really does answer "whose
network simplex is faster". Boost has no network simplex (it offers cycle cancelling and
successive shortest paths), so it is absent rather than losing.

Four instance families, because they disagree about which library wins and any one of them
alone would be a misleading result. `netgen` is the structure where a source-packed arc order
hurts the block rule most; `assignment` (k×k, unit supplies and capacities) is the opposite
extreme, where the pivot count is tiny next to the arc count and setup cost decides it;
`transport` is the classic dense-bipartite OR shape, long enough that the entering-arc scan
dominates; `circulation` has zero supplies and 30% negative-cost arcs, and is the only family
here that exercises negative costs at all. Setup is inside the timed region on both sides —
LEMON's constructor building its internal copy, MELON's `reset()` — because for short solves
that *is* the cost.

What separates them on `netgen` is one feature, **arc mixing**, so it is swept rather than fixed:
it is a benchmark parameter like the value type, and each chart is one setting (`int/mixed`,
`int/unmixed`, ...) with the containers compared inside it as usual. LEMON copies the arcs into its own
arrays in a scattered order, so each block-search block samples many source vertices; MELON has
the same permutation as a *scan* order over its arcs (a traits flag, off by default), and its
`mutable_digraph` is charted unmixed only, since a strided visit needs random access to the arcs
range and a join over per-vertex out-arc lists does not give it. Unmixed, both scan the order the
graph stores, and both MELON containers group arcs by source (87.5% of consecutive arcs share a
source on these instances). Correlated blocks make worse entering-arc choices, and unmixed MELON
needs 1.4×–2.1× the pivots as a result. Two measurements pin it: turning LEMON's mixing off
doubles its time on the largest `netgen` instance (25.1 ms → 48.2 ms, landing next to unmixed
MELON), and giving both the order-independent Dantzig rule instead makes their pivot counts
identical to within 1%. Everything else — the basis representation, the leaving-arc rule and
Cunningham's tie-break, the artificial cost, the block size, the cursor mechanics — measured as no
difference at all.

The same packing that costs MELON pivots on `netgen` is what makes its scan cheap elsewhere: it
reads `pi[source]` in runs of one vertex where LEMON's mixed order randomises both endpoint
gathers. On `assignment` that reverses: mixing costs the same container more time than it saves on
`netgen`. Which effect wins is a property of the instance, which is why there are four families
and not one, and why both settings are charted rather than one picked.

Adapters for [CXXGraph](https://github.com/ZigRazor/CXXGraph) exist under `src/cxxgraph/`
but are disabled: its traversals are O(|V|·|A|) (visited set scanned linearly per edge), so
only the 3.4 K-vertex `rome99` instance is feasible, and its Dijkstra only answers
point-to-point queries.

## Reproducing

```sh
pip install -r requirements.txt
make data                      # ~800 MB of third-party graphs into data/
make                           # build -> benchmark -> validate -> plot
make web                       # aggregate all runs into the web/ chart viewer
```

`make benchmark` takes about half an hour. Roughly 1200 benchmark cases each cost at least
`--repetitions x --min-time`, so those two defaults (5 and `0.1s`) are what the runtime is
made of rather than the measurements themselves -- four fifths of the cases run in under
20 ms. Both are flags on `benchmark.py` if a longer, steadier run is wanted.

Requirements: Conan 2, CMake ≥ 3.12, Python 3. Each library is a self-contained Conan/CMake
project under `src/<library>/`, so libraries never share a translation unit or fight over
dependency versions; dependency versions are pinned and recorded, with build flags and
hardware facets, in `results/<run id>/_provenance.json`. Two Conan profiles are expected
(`gcc14_c++23` for MELON, `gcc14_c++20` for the baselines) — same compiler, different
`compiler.cppstd`:

```ini
[settings]
os=Linux
arch=x86_64
compiler=gcc
compiler.version=14
compiler.cppstd=23        # 20 for the gcc14_c++20 profile
compiler.libcxx=libstdc++11
build_type=Release
[conf]
tools.build:compiler_executables={"c": "gcc-14", "cpp": "g++-14"}
```

`make NATIVE=ON` builds and benchmarks a separate `-march=native` configuration in its own
build and results directories, so generic and native numbers can never mix. Datasets come
from their original publishers: [SNAP](https://snap.stanford.edu/data/) directed graphs, the
[9th DIMACS Implementation Challenge](http://www.diag.uniroma1.it/challenge9/download.shtml)
USA road networks, [BVZ-tsukuba](https://vision.cs.uwaterloo.ca/data/maxflow) max-flow
instances, locally generated Goldfarb–Grigoriadis RMF max-flow instances with
independently cross-checked `.sol` optima, and four locally generated min-cost-flow families
(NETGEN, assignment, transportation, circulation). Their `.sol` optima carry a verified dual
certificate — feasibility plus non-negative reduced costs on every residual arc, which is a
proof rather than a second opinion. The largest instance of each family exceeds what the
Python reference can certify in reasonable time and ships without one; there the gate is
`validate.py`'s cross-library digest agreement, as it already is for the SNAP max-flow
instances. Generating them takes about three minutes, nearly all of it in that solver.

## Known limitations

1. **Measurement environment.** The recorded runs had CPU frequency scaling enabled and
   non-trivial load average; error bars show the resulting dispersion rather than hiding it.
2. **Generic codegen** for the published numbers (`-march=native` runs are kept separately).
3. **`kruskal` int-vs-double digest mismatch** on the four largest road networks (see
   [Validation](#validation)); kruskal timings are withheld until it is resolved.
4. **Bellman-Ford instance range.** Its `n*m` work makes one run over the three largest
   road networks cost tens of seconds, so those instances are skipped for it and for it
   only (`instance_is_affordable` in `include/helper.hpp`). The three smaller ones order the
   libraries and the containers the same way.
5. **Source-set bias.** Sources are evenly spaced vertex ids; on road networks ids are
   spatially correlated, so the sample is not uniform over the graph. It is identical
   across libraries, and the digests prove it.

## License

Not yet specified.

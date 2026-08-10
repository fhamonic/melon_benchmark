# MELON benchmark

Runtime comparison of [MELON](https://github.com/fhamonic/melon) against other C++
graph libraries — [LEMON](https://lemon.cs.elte.hu/), [Boost.Graph](https://www.boost.org/doc/libs/release/libs/graph/),
[CXXGraph](https://github.com/ZigRazor/CXXGraph), and (not yet wired up) [CGAL](https://www.cgal.org/).

Each library gets its own self-contained CMake project and Conan recipe under `src/<library>/`,
so libraries never share a translation unit and never fight over dependency versions. Every
benchmark binary is a [Google Benchmark](https://github.com/google/benchmark) main that registers
one benchmark per (instance, graph container, value type, …) combination and publishes a digest of
its result. `validate.py` checks every library agreed on the answer; only then does `plot.py` draw
anything.

## Results

Every number below is the median of 5 repetitions on `USA-road-d.NE` (1.5 M vertices, 3.9 M arcs),
each library in its fastest container, built with the same compiler. Every configuration in every
row produces a byte-identical result digest — see [Validation](#validation) — so nothing here is
winning by computing less.

> **Stale numbers.** Only the point-to-point table below was measured against the current MELON.
> Every other MELON figure on this page predates a MELON update that renamed
> `views::element_map` → `maps::element_map`, renamed the `dijkstra_trait` concept to
> `dijkstra_traits`, and reworked `bidirectional_dijkstra` (`run()` now returns `*this` and the
> distance comes from `dist()`). MELON's absolute timings moved with it — the point-to-point
> unidirectional figure on `USA-road-d.NE` went from 107.6 ms to 85.8 ms — so the LEMON and Boost
> columns are still valid but the MELON ones and every ratio derived from them are not. Run
> `make benchmark validate plot` to refresh; `results/_provenance.json` records which MELON package
> each JSON came from.

### Throughput

| benchmark | MELON | LEMON | Boost |
|---|---:|---:|---:|
| `breadth_first_search` | **10.0 ms** | 30.8 ms (3.1×) | 26.3 ms (2.6×) |
| `depth_first_search` | **20.2 ms** | 36.3 ms (1.8×) | 47.4 ms (2.3×) |
| `dijkstra` `int/4-heap` | **76.2 ms** | 113.3 ms (1.5×) | 131.1 ms (1.7×) |
| `strongly_connected_components` | **28.7 ms** | 86.8 ms (3.0×) | 83.2 ms (2.9×) |
| `traversal_forest` | **10.7 ms** | 28.4 ms (2.7×) | 59.3 ms (5.5×) |
| `weakly_connected_components` | **37.8 ms** | 67.0 ms (1.8×) | — (none correct) |

### Point-to-point: bidirectional search

`dijkstra_point_to_point` asks each library for the distance between a source and one target, over
the same deterministic source/target pairs. It exists because that is the only shortest-path
question CXXGraph's API can answer, but it is also where MELON has an algorithm the other three do
not: `bidirectional_dijkstra`, which grows a search from each end and stops when they meet.

MELON is registered twice there. The `:unidirectional` bars are the same-algorithm comparison
against LEMON, Boost and CXXGraph; the `:bidirectional` bar answers "what does this library give
you for this query". Both produce the same distances — the digest enforces it.

| instance | \|V\| | unidirectional | bidirectional | speedup |
|---|---:|---:|---:|---:|
| `rome99` | 3.4 K | 76.9 µs | 72.8 µs | 1.06× |
| `USA-road-d.NY` | 264 K | 6.61 ms | 5.34 ms | 1.24× |
| `USA-road-d.COL` | 436 K | 6.83 ms | 5.46 ms | 1.25× |
| `USA-road-d.NE` | 1.52 M | 85.8 ms | 56.3 ms | **1.52×** |

The gain grows with the graph, as it should: bidirectional search wins by exploring less area, and
there is more area to save on a bigger network. It stays below the textbook ~2× because it
allocates two heaps and two vertex-status maps per query instead of one, and the storage-policy
section shows per-query setup is what dominates a point-to-point query.

### Short searches: what MELON's traits actually buy

MELON is the only one of the three whose Dijkstra can be told not to build the shortest-path tree.
Because Dijkstra's per-query setup is O(|V|) however far the search runs, that matters enormously
for queries that stop early and not at all for queries that cover the graph. The `dijkstra_bounded`
benchmark is a k-nearest-vertices query — consume each distance as the vertex is settled, then stop
— with every library in the leanest configuration its own API allows (µs **per query**):

| k | MELON streaming | MELON stored | LEMON streaming | Boost streaming |
|---:|---:|---:|---:|---:|
| 100 | **15 µs** | 206 µs (14.2×) | 322 µs (22.2×) | 1 065 µs (73.4×) |
| 1 000 | **24 µs** | 223 µs (9.2×) | 337 µs (13.9×) | 1 093 µs (45.0×) |
| 10 000 | **334 µs** | 550 µs (1.6×) | 818 µs (2.4×) | 1 592 µs (4.8×) |
| unbounded | **68.8 ms** | 75.4 ms (1.1×) | 117.6 ms (1.7×) | 136.0 ms (2.0×) |

> For short searches on large graphs — service areas, k-nearest, any query that consumes distances
> online and stops — a MELON query costs one to two orders of magnitude less than the same query
> written against LEMON's or Boost's APIs. As the search grows to cover the graph that advantage
> amortizes away to about 10% over MELON's own storing mode, and MELON leads the other two by a
> more ordinary 1.7–2.0×.

**Read that as per-query cost, not traversal speed.** At k = 100 roughly 83% of the measured time
is constructing the algorithm object, not searching — splitting the two on `USA-road-d.NE` gives
13.3 µs to construct and 2.7 µs to settle 100 vertices in streaming mode, against 205.8 µs and
8.0 µs stored. What the short-k rows measure is the per-query setup a caller cannot avoid paying,
and MELON's is ~15× cheaper than its own storing mode; they are not a claim that MELON's inner loop
is 73× faster. Setup is linear in |V| in both modes (264 K → 1.5 M vertices scales streaming
construction 2.0 → 13.3 µs), just far cheaper per vertex when the shortest-path tree is not built.

Publishing the whole curve rather than the k = 100 row is the point: the 73× is real, and so is the
1.1×, and a reader who only saw the first would rightly distrust it. The per-trait breakdown
(`store_paths` is the entire cost, `store_distances` is free) and the caveats are in
[Storage policy](#storage-policy-what-store_distancesstore_paths-actually-buy).

## Quick start

```sh
pip install -r requirements.txt
scripts/fetch_data.sh          # ~800 MB of third-party graphs
make                           # build -> benchmark -> validate -> plot
```

Individual stages:

```sh
make build      # build all library benchmark binaries into build/<config>/
make benchmark  # run binaries, write results/<run id>/<library>-<algorithm>-<dataset>.json
make validate   # assert every library computed the same answer
make plot       # render plots/<run id>/<algorithm>[_<params>]_<dataset>.png
make web        # aggregate ALL runs under results/ into web/data.json
make clean      # remove the current configuration's build, results and plots
```

`web/index.html` is a static [ECharts](https://echarts.apache.org/) viewer over `web/data.json`
(vendored `echarts.min.js`, no CDN): one selector per run facet plus algorithm / dataset /
parameters, log or linear scale, a table view, and light/dark themes. Preview locally with
`python3 -m http.server -d web` after `make web`. Every selector mirrors into the URL, so once
`web/` is published (e.g. GitHub Pages), a documentation page can embed a specific chart:

```html
<iframe src="https://<pages-url>/?algorithm=dijkstra&dataset=9th_dimacs&params=int%7C4-heap"
        loading="lazy" style="width:100%;height:560px;border:0"></iframe>
```

A **run** is one (machine, compiler, build options) combination, keyed as
`results/<host>_<compiler>_<config>/` — e.g. `results/voltron-unleashed_gcc14_generic/`. Each run
directory carries its own `_provenance.json` whose `facets` dict (`hardware`, `compiler`,
`options`, plus anything passed as `benchmark.py --facet KEY=VALUE`) labels the run for downstream
selectors. Runs from different machines or configurations live side by side and are all folded
into `web/data.json` by `make web`.

`make NATIVE=ON …` builds and benchmarks a second configuration with `-march=native`, in its own
`build/native/` and `results/<host>_<compiler>_native/`, so generic and native numbers can never
mix. The flag is passed to CMake as a *cache* entry and `benchmark.py` cross-checks the recorded
`OPTIMIZE_FOR_NATIVE` against the run's `options` facet — a run whose label contradicts its build
caches is refused. Caveat: the flag applies to the benchmark translation units; Conan-built
dependencies keep their generic binaries. That is immaterial for the header-only libraries (MELON,
Boost.Graph, CXXGraph) but LEMON's compiled parts stay generic.

Requirements: Conan 2 with the profiles below, CMake ≥ 3.12, Python 3 with `matplotlib` and
`numpy`, and the datasets in `data/`.

**Binaries resolve dataset paths relative to the current working directory**, so all commands must
be run from the repository root. `make` does this for you.

### Conan profiles

Every library is built with the **same compiler**; only the language standard differs, because
MELON requires C++23 and the baselines do not compile as C++23. This matters: building the
libraries under different compiler *versions* silently attributes GCC codegen differences to the
libraries, which is the easiest way to produce a flattering and meaningless chart.

The profiles are not checked in — create `gcc14_c++23` and `gcc14_c++20` under `~/.conan2/profiles/`:

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

To use a different toolchain, override the two variables at the top of the `Makefile`.

### Running one library, or one benchmark

```sh
conan build src/melon -of=build/generic/melon -b=missing -pr=gcc14_c++23
./build/generic/melon/build/Release/dijkstra-9th_dimacs --benchmark_filter='USA-road-d.NY/'
```

`benchmark.py` accepts `--repetitions`, `--min-time`, `--filter` and `--force`. Useful Google
Benchmark flags on the binaries themselves: `--benchmark_filter=<regex>`,
`--benchmark_repetitions=N`, `--benchmark_min_time=Ns`.

Use `make NATIVE=ON` to add `-march=native` (off by default, so the shipped numbers are generic
x86-64); see [Quick start](#quick-start) for how that keys a separate configuration. LTO is always
on via `cmake/CompilerOptimizations.cmake`.

## Layout

```
include/                     shared across libraries
  checksum.hpp                 result digests -- the basis of cross-library validation
  max_flow_reference.hpp       DIMACS terminals + the shipped .sol reference flow values
  helper.hpp                   instance_sources(): source set sized by algorithm complexity
  {dimacs,snap,bvz_tsukuba}_instances.hpp   (path, |V|, |A|) triples
  generate_segments.hpp        random segments for the Bentley-Ottmann benchmark
src/<library>/
  conanfile.py               dependencies for that library only
  CMakeLists.txt             one executable per <algorithm>/<dataset>.cpp
  checksums.hpp              that library's adapters onto include/checksum.hpp
  parse_*.hpp                library-specific readers, one per graph container
  <algorithm>/<dataset>.cpp  the benchmark itself
cmake/                       shared warning + optimization flags
scripts/fetch_data.sh        download and normalize the datasets
scripts/generate_rmf.py      generate data/rmf + .sol references + rmf_instances.hpp
benchmark.py                 run binaries -> results/<run id>/ (+ _provenance.json with facets)
validate.py                  assert all libraries agree; non-zero exit stops the build
bench_results.py             which rows count, and how a row becomes milliseconds --
                             shared by plot.py and export_web.py so they cannot drift
plot.py                      results/<run id>/ -> plots/<run id>/
export_web.py                every run under results/ -> web/data.json (for the docs viewer)
web/                         static ECharts viewer (index.html + vendored echarts.min.js);
                             data.json is generated, not tracked
build/<config>/<library>/    one build tree per option set (generic, native)
data/                        datasets (not tracked, ~800 MB)
results/<run id>/            tracked, so published numbers stay reproducible; one
                             directory per (machine, compiler, build options) run
```

Binary names encode `<algorithm>-<dataset>`; `benchmark.py` prepends the library to produce
`results/<run id>/<library>-<algorithm>-<dataset>.json`. Benchmarks are named
`<instance>/<container>[/<param>…]`, and `plot.py` draws one chart per (algorithm, params, dataset)
with one bar series per `<library>::<container>`. Run directory names are opaque keys; the labels
shown to humans come from the `facets` in each run's `_provenance.json`.

Because of that convention, **library, algorithm and dataset names must not contain `-`**.

## Validation

This is the part that makes the timings mean anything.

Every benchmark computes its result once, outside the timed region, reduces it to a 64-bit digest
and publishes it as the run's Google Benchmark label. `validate.py` groups runs by (algorithm,
dataset, instance) and asserts that **every library, every graph container and every value type
produced the same digest**. `make plot` depends on `make validate`, so a disagreement stops the
build instead of becoming a bar in a chart.

Digests are canonical by construction:

- Values are read in **vertex id order**, never traversal order — MELON's `mutable_digraph` walks
  an intrusive list and yields vertices in reverse creation order, which would otherwise make the
  digest depend on the container.
- Partitions (connected components, SCC, traversal forest) are canonicalized before hashing: for
  each vertex, the smallest vertex id in its component. Component *numbering* is an implementation
  detail; the partition is not.
- Everything is quantized to 1/1024 before mixing, so `5` and `5.0` hash identically and the `int`
  series of an algorithm is checked against its `double` series.

For max flow there is an absolute check on top: BVZ-tsukuba ships the optimal flow value in a
sibling `.sol` file, and each run compares against it and calls `SkipWithError` on a mismatch.

The digest is computed once outside the timed region, so on its own it proves the *validation* pass
did the work, not the timed loop. `dijkstra_bounded` — where a query can finish in 15 µs and
reasonably invite disbelief — therefore also counts vertices *inside* the timed loop and reports
them as a `settled` counter. It must read `min(k, reachable)`; anything else means the loop was
partly optimized away. It reads exactly 100 at `k100` and 1 524 453 at `kall` for all three
libraries. That counter is worth having: it immediately caught Boost reporting `settled=0`, because
BGL's named-parameter mechanism takes the visitor **by value** and the count was accumulating on a
copy.

This machinery is not decorative — it caught three real defects, listed under
[What was fixed](#what-was-fixed).

## What is measured

Parsing and graph construction happen **outside** the timed region. Inside it:

- **Single-source algorithms** (`dijkstra`, `breadth_first_search`, `depth_first_search`) run once
  per source in a source set. The set is `instance_sources(n, m, complexity)`: evenly spaced vertex
  ids, sized so total work is roughly a constant `1e8` complexity budget, so small and large
  instances take comparable wall time. The algorithm object is constructed inside the loop, so
  per-query setup (heap allocation, map initialization) is included — deliberately, it is part of
  the cost of answering a query.
- **Whole-graph algorithms** (`strongly_connected_components`, `weakly_connected_components`,
  `traversal_forest`, `kruskal`, `max_flow`) run once per iteration.

All three libraries do the **same bookkeeping**. For Dijkstra that means each keeps a distance map
and a predecessor map, allocated per query, and consumes each vertex's distance as it is settled.
MELON is faster with `store_distances`/`store_paths` disabled — a real advantage of its traits —
but measuring MELON with the bookkeeping off against LEMON and Boost with it on is not a
comparison, so the benchmark does not do that.

`benchmark::DoNotOptimize` guards every produced value; `benchmark::MaybeReenterWithoutASLR` runs
first to reduce layout-induced variance. `benchmark.py` defaults to 10 repetitions and plots the
median with standard-deviation error bars.

### Storage policy: what `store_distances`/`store_paths` actually buy

The `dijkstra` chart deliberately makes all three libraries store distances and predecessors,
because otherwise it would not be a comparison. But MELON's traits let a caller say "I do not need
either of those", and that is a real capability worth measuring — so `dijkstra_bounded` measures
it, as a task rather than as a configuration flag.

The task is a **k-nearest-vertices query**: settle the k closest vertices to a source, consume each
distance as the vertex comes off the heap, then stop. Every library is registered twice per k, as
`<container>:stored` and `<container>:streaming`, each in the leanest configuration its own API
allows:

| | drops predecessor map | drops distance map | early exit |
|---|---|---|---|
| MELON | `store_paths = false` | `store_distances = false` | `break` out of the range-for |
| LEMON | `PredMap = NullMap` | `DistMap = NullMap`, read the heap via `currentDist` | `break` out of the loop |
| Boost | omit `predecessor_map` | **cannot** — the distance map *is* the heap's key | throw from the visitor |

LEMON gets `NullMap` for its predecessor, distance *and* processed maps, which is legitimate
because `processNextNode()` only ever writes them; handicapping it would make MELON's advantage
look bigger than it is. Boost's distance map is not an API choice —
`d_ary_heap_indirect<Vertex, 4, IndexInHeapMap, DistanceMap, …>` is keyed on it.

**k is the variable that matters**, and publishing the curve rather than one point on it is the
whole idea. Dijkstra's per-query setup is O(|V|) no matter how far the search runs, so maps a
library cannot avoid allocating are amortized away when k is large and dominate when k is small.
The measurements are in [Results](#short-searches-what-melons-traits-actually-buy); what follows is
why they come out that way.

**The cost is `store_paths`, not `store_distances`.** Isolating the two traits on the same instance
(µs per query):

| k | both | distances only | paths only | neither |
|---:|---:|---:|---:|---:|
| 100 | 213 | 16 | 212 | 22 |
| 1 000 | 239 | 47 | 237 | 55 |
| 10 000 | 562 | 369 | 543 | 383 |
| 100 000 | 4 231 | 3 996 | 4 120 | 4 044 |

`store_paths` costs ~190 µs per query at any k; `store_distances` costs nothing measurable. The
cost is flat in k, which is what identifies it as construction rather than search — timing the
constructor alone reproduces it (205.8 µs stored vs 13.3 µs streaming on `USA-road-d.NE`).

Counting real allocations through a replaced global `operator new` confirms what those two traits
do, and confirms that nothing is elided at compile time despite MELON being `constexpr` throughout
(C++14 onward permits the compiler to elide allocations, so this is worth checking rather than
assuming). One k = 100 query on `USA-road-d.NE`:

| | allocations | bytes | settled | sum of distances |
|---|---:|---:|---:|---:|
| streaming | 8 | 13.08 MB | 100 | 484366 |
| stored | 10 | 30.53 MB | 100 | 484366 |

The difference is exactly two allocations and 17.45 MB, which over 1 524 453 vertices is 12 bytes
each: a 4-byte `int` distance map plus an 8-byte `std::optional<arc>` predecessor map. Only the
second is value-initialized, which is why it carries the whole cost. The practical advice follows:
**if you need distances but not paths, setting `store_paths = false` alone gets you essentially the
whole win.**

Two further checks that the timings are not a compile-time artifact. Supplying k through a
`volatile` so the compiler cannot know it changes nothing (streaming 16.2 µs with the template
parameter vs 15.7 µs through the volatile; stored 210.8 vs 216.9 µs). And the `settled` counter,
accumulated inside the timed loop, reads exactly `min(k, reachable)` for every library.

Three caveats worth stating. The advantage depends on constructing a fresh algorithm object per
query, which is what all three benchmarks do — MELON's `reset()` refills the status map and would
behave like the storing mode. Because the short-k rows are dominated by allocation, they are also
sensitive to allocator behaviour: glibc raises its mmap threshold once it sees repeated frees of
large blocks, so a steady-state benchmark loop sees warmer pages than a cold first call would. That
applies equally to all three libraries, which all allocate per query. And LEMON's streaming mode is
slightly *slower* than its storing mode at `kall` (117.6 ms vs 111.4 ms per query), because
`currentDist(v)` costs an extra heap probe per settled vertex, which outweighs the saved map writes
once the search covers the graph.

### On `traversal_forest` vs `weakly_connected_components`

These are two different benchmarks on purpose:

- `weakly_connected_components` computes the genuine weak components of a digraph. LEMON does it
  correctly via `Undirector`; Boost has no correct equivalent, so there is no Boost series.
- `traversal_forest` computes the partition induced by repeated traversal from unvisited vertices.
  This is what `lemon::connectedComponents` and `boost::connected_components` *actually* compute
  when handed a directed graph — which is not the weak components.
  `src/{lemon,boost}/weakly_connected_components/test.cpp` are four-vertex reproducers
  demonstrating that. MELON exposes the operation under its honest name, `melon::traversal_forest`.

So the `traversal_forest` chart compares equivalent work, even though the Boost and LEMON entry
points are named `connected_components`.

It is the one algorithm here whose **result is not a function of the graph alone**. The forest
depends on the order unvisited vertices are chosen as roots, and that order is a property of the
container: MELON's `static_digraph` and Boost's CSR iterate ids ascending, while MELON's
`mutable_digraph` (intrusive list) and LEMON's `NodeIt` descend. All are correct; they produce
different forests. `validate.py` therefore reports the digest classes for `traversal_forest`
instead of requiring one — and the check still bites within a root order, where MELON's
`static_digraph` and Boost's CSR must agree, and do. Every other algorithm collapses to a single
digest across all three libraries.

## Coverage

| Algorithm | Dataset | MELON | LEMON | Boost | CXXGraph |
|---|---|---|---|---|---|
| `dijkstra` | 9th_dimacs | static/mutable × {int,double} × {2,4,8}-heap | Static/Smart/List × {int,double} × {2,4,8}-heap | adjacency_list + CSR × {int,double}, 4-ary heap only | ❌ no single-source API |
| `dijkstra_point_to_point` | 9th_dimacs | static/mutable × {unidirectional, **bidirectional**} | Static/Smart | CSR | rome99 only |
| `dijkstra_bounded` | 9th_dimacs | static/mutable × {stored,streaming} × k | Static/Smart × {stored,streaming} × k | CSR × {stored,streaming} × k | ❌ |
| `breadth_first_search` | snap, 9th_dimacs | ✅ | ✅ | ✅ | rome99 only |
| `depth_first_search` | snap, 9th_dimacs | ✅ | ✅ | ✅ | rome99 only |
| `strongly_connected_components` | snap, 9th_dimacs | ✅ | ✅ | CSR only | ❌ |
| `weakly_connected_components` | snap, 9th_dimacs | ✅ | ✅ | ❌ none correct | ❌ |
| `traversal_forest` | snap, 9th_dimacs | ✅ | ✅ | CSR only | ❌ |
| `max_flow` | bvz_tsukuba | dinitz | preflow | push_relabel, boykov_kolmogorov | ❌ |
| `max_flow` | rmf | dinitz | preflow | push_relabel, boykov_kolmogorov | ❌ |
| `max_flow` | snap | dinitz | preflow | ❌ | ❌ |
| `kruskal` | 9th_dimacs, bvz_tsukuba | ✅ | ✅ | ❌ | ❌ |
| `bentley_ottmann` | random segments | bounded8/16/32 × int64 × int128 | — | — | — |

`bentley_ottmann` is the one benchmark where the baseline is **CGAL** (its `epeck` exact-kernel
surface-sweep, `src/cgal/`), not a graph library. Two seeded families per size n ∈ {16 … 1024}:
`box_n<k>` (endpoints uniform in the box — long, intersection-dense segments) and `vec_n<k>`
(anchor plus short vector — local, sparse). The coordinate range (`8bit`/`16bit`) is the task
parameter; melon's series are its coordinate value types, and every type must produce the same
event-count digest as CGAL. Two semantic traps are handled in the generator and worth knowing
about: melon's `report_endpoints` and CGAL's are *different flags* (melon's reports intersections
lying at endpoints, CGAL's reports every endpoint; CGAL's default `false` setting is what matches
melon), and segments sharing an endpoint are excluded at generation because that is the one event
class the two libraries cannot be configured to report identically.

### CXXGraph

CXXGraph is benchmarked on `breadth_first_search`, `depth_first_search` and
`dijkstra_point_to_point`, and only on the `rome99` instance (3 353 vertices). Both limits are
properties of the library, not choices:

- **Its Dijkstra answers a different question.** `dijkstra(source, target)` returns a single
  `double` for one source/target pair, so it cannot appear in the `dijkstra` chart, which asks the
  single-source question. `dijkstra_point_to_point` exists so that it can be compared at all: every
  library answers "distance from s to t" there, with the same deterministic source/target pairs.
- **Only the smallest instance.** Both traversals keep the visited set in a `std::vector` and run
  `std::find` over it for every edge examined, making them O(|V|·|A|) rather than O(|V|+|A|). One
  BFS on `USA-road-d.NY` — the *smallest* USA network here — takes ~38 s against MELON's ~16 ms,
  and the larger networks are out of reach. DFS additionally recurses once per settled vertex, so
  on any USA network the stack depth would reach hundreds of thousands.

`rome99` was added to the shared DIMACS instance list for this, so all four libraries run it and
the comparison on it is real; it costs the other three microseconds. CXXGraph's digests match
MELON's, LEMON's and Boost's on all three benchmarks, so it is solving the same problems.

Per query on `rome99`, the gap is very different for the two kinds of algorithm:

| | MELON | LEMON | Boost | CXXGraph |
|---|---:|---:|---:|---:|
| `breadth_first_search` | **12.1 µs** | 47.0 µs | 41.8 µs | 3 170 µs (262×) |
| `depth_first_search` | **13.5 µs** | 41.7 µs | 59.8 µs | 3 197 µs (237×) |
| `dijkstra_point_to_point` | **72.9 µs** | 94.3 µs | 103.7 µs | 1 718 µs (23.6×) |

The distinction is worth stating plainly: CXXGraph's **Dijkstra is algorithmically sound** — a
proper binary-heap implementation, O((|V|+|A|) log |V|) — and its 23.6× is constant-factor cost,
from `shared_ptr` indirection, `std::string` node ids, a distance map keyed by `shared_ptr`, and an
inability to stop at the target (it runs `while (!pq.empty())` to exhaustion and then reads
`dist[target]`, so a point-to-point query costs a full single-source computation). Its **traversals
are algorithmically wrong for the job** — quadratic — and that is where the ~250× comes from. A
single "CXXGraph is N× slower" number would have hidden the difference between the two.

Max-flow runs register as `<instance>/<container>:<algorithm>/<value>`, so every library's
max-flow algorithms share one chart. Boost's `boykov_kolmogorov` is included because BVZ-tsukuba
is the vision dataset that algorithm was published to solve — and on it, it is roughly an order of
magnitude faster than everything else, which a Dinitz-only chart would never have shown.


## What was fixed

The validation layer was added to an existing benchmark suite. It immediately found three defects
that had been silently shaping the published charts:

1. **Boost's `double` Dijkstra was not double-precision.** The distance map was declared
   `std::vector<int>` for both value types, so Boost deduced `closed_plus<int>` and the `double`
   series accumulated truncated integer arithmetic. It is now `std::vector<_Value>`, and its digest
   matches MELON's and LEMON's.
2. **Boost's Dijkstra was charted against the wrong heap.** It was registered as `2-heap`, but
   `boost::dijkstra_shortest_paths` defaults to `d_ary_heap_indirect` with **arity 4**. Its bars
   were being compared against MELON's and LEMON's *binary* heaps. Now registered as `4-heap`.
3. **Boost paid for an O(n) sweep its competitors did not.** Its Dijkstra loop read all |V|
   distances back *after* each run, inside the timed region, while MELON and LEMON consumed
   distances as vertices were settled. Boost now uses an `examine_vertex` visitor, matching them.

Two smaller ones: the CSR reader parsed weights as `double` regardless of the graph's value type,
and the DIMACS max-flow terminals were hardcoded to vertices 0 and 1 rather than read from the
file's `n <id> s` / `n <id> t` lines.

## Known limitations

1. **Measurement environment.** The recorded runs had `cpu_scaling_enabled: true` and non-trivial
   load average. Error bars now show the resulting dispersion rather than hiding it, but pinning
   frequency and running on an idle machine would produce tighter numbers.
2. **Generic codegen.** `OPTIMIZE_FOR_NATIVE` defaults to `OFF`.
3. **`max_flow` compares different algorithms.** Dinitz, preflow, push-relabel and
   Boykov-Kolmogorov are not the same algorithm; the chart answers "what does each library give
   you", not "whose Dinitz is faster". Edmonds-Karp is the one algorithm all three libraries have,
   but it is registered-and-commented-out: its O(V·E²) bound makes it one to two orders of
   magnitude slower than everything else here, so it dominated the runtime of a full `make` without
   changing any conclusion. Re-enable the `REGISTER` lines in `src/*/max_flow/` to get the
   same-algorithm row back.
4. **Source-set bias.** `instance_sources` picks evenly spaced *vertex ids*. On road networks ids
   are spatially correlated, so the sample is not uniform over the graph. It is at least identical
   across libraries, and the digests prove it.
5. **Hardcoded instance sizes.** `include/*_instances.hpp` hardcodes |V| and |A| per instance and
   nothing cross-checks them against the files; they size the source set. Swap in a different
   instance and update them, or the benchmark silently measures a different amount of work.
   (`rmf_instances.hpp` is the exception: `scripts/generate_rmf.py` writes it from the graphs it
   actually generated.)
6. **No CI.** Nothing builds all four projects on push, which is how the `edmonds_karp` and
   `kruskal` targets came to be commented out and drift.

## Datasets

`data/` is not tracked. `scripts/fetch_data.sh [snap|dimacs|bvz|rmf|all]` downloads everything
from its original publisher (`rmf` is generated locally, see below) and normalizes the SNAP files
into the `<|V|> <|A|>` + 0-indexed edge-list format the benchmarks expect.

| Dataset | Path | Source |
|---|---|---|
| SNAP directed graphs | `data/snap/*.txt` | [SNAP](https://snap.stanford.edu/data/) — `web-Stanford`, `Amazon0302`, `Amazon0505`, `WikiTalk` |
| 9th DIMACS USA road networks | `data/9th_DIMACS_USA_roads/{distance,time}/*.gr` | [9th DIMACS Implementation Challenge](http://www.diag.uniroma1.it/challenge9/download.shtml) |
| BVZ-tsukuba max-flow | `data/BVZ-tsukuba/BVZ-tsukuba*.{max,sol}` | [Computer Vision max-flow problems](https://vision.cs.uwaterloo.ca/data/maxflow) |
| RMF max-flow family | `data/rmf/rmf_{long,wide}_*.{max,sol}` | generated by `scripts/generate_rmf.py` (Goldfarb–Grigoriadis structure, deterministic seeds) |

Keep the `.sol` files: they are the reference max-flow values the benchmarks check against. The
RMF ones are computed by the generator itself with Dinic's algorithm, cross-checked against an
independent Edmonds–Karp run, and the generator was itself validated by reproducing
BVZ-tsukuba0's shipped optimum. The generator also writes `include/rmf_instances.hpp` from the
graphs it actually produced, so for this dataset the instance list cannot drift from the data
(unlike the hand-maintained lists, see [Known limitations](#known-limitations) 5). BVZ-tsukuba is
push-relabel's and Boykov–Kolmogorov's home turf; the RMF family is the classic DIMACS stress
structure where the algorithm ranking is allowed to come out differently — publishing both is the
same idea as publishing the whole `dijkstra_bounded` curve.

## License

Not yet specified.

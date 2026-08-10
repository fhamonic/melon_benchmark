"""Generate RMF-family max-flow instances, reference solutions, and the
instance list header.

Usage: python scripts/generate_rmf.py   (from the repository root)

Writes data/rmf/<name>.max (DIMACS max-flow format), a sibling <name>.sol with
the optimal flow value, and include/rmf_instances.hpp with the exact (path,
|V|, |A|) of what was generated -- so the header cannot drift from the data,
unlike the hand-maintained instance lists (README, "Known limitations" 5).

The networks follow the RMF structure of Goldfarb & Grigoriadis (the DIMACS
"genrmf" family): b frames of a*a grid nodes; within a frame, 4-neighbor arcs
in both directions with the large capacity c2*a*a; between consecutive frames,
a seeded random permutation matches each node to one node of the next frame
with a random capacity in [c1, c2]. Source is the first corner of the first
frame, sink the last corner of the last frame, so the flow must cross every
frame boundary and the min cut lives between frames. "long" instances (small
a, many frames) and "wide" ones (large a, few frames) stress augmenting-path
and push-relabel algorithms differently, which is the point of adding them
next to the vision-structured BVZ instances: this is not an implementation of
DIMACS genrmf, but it is the same family.

Everything is deterministic: each instance seeds its own RNG with a hash of
its name. Reference values are computed here with Dinic's algorithm and
cross-checked with an independent Edmonds-Karp run; the benchmarks then check
their flow against the .sol and their digests against each other.
"""

import hashlib
import random
import sys
from collections import deque
from pathlib import Path

C1, C2 = 1, 1000

# (name, a, b) -- a*a nodes per frame, b frames.
INSTANCES = [
    ("rmf_long_a8_b64", 8, 64),
    ("rmf_long_a11_b128", 11, 128),
    ("rmf_wide_a24_b12", 24, 12),
    ("rmf_wide_a40_b12", 40, 12),
]


def build_rmf(a, b, rng):
    """Returns (num_nodes, arcs) with arcs as (u, v, capacity), 0-indexed."""
    def node(frame, i, j):
        return frame * a * a + i * a + j

    arcs = []
    in_frame_capacity = C2 * a * a
    for frame in range(b):
        for i in range(a):
            for j in range(a):
                u = node(frame, i, j)
                if i + 1 < a:
                    v = node(frame, i + 1, j)
                    arcs.append((u, v, in_frame_capacity))
                    arcs.append((v, u, in_frame_capacity))
                if j + 1 < a:
                    v = node(frame, i, j + 1)
                    arcs.append((u, v, in_frame_capacity))
                    arcs.append((v, u, in_frame_capacity))
        if frame + 1 < b:
            targets = list(range(a * a))
            rng.shuffle(targets)
            for v_in_frame, target in enumerate(targets):
                arcs.append((
                    frame * a * a + v_in_frame,
                    (frame + 1) * a * a + target,
                    rng.randint(C1, C2),
                ))
    return a * a * b, arcs


class Dinic:
    def __init__(self, n):
        self.n = n
        self.adj = [[] for _ in range(n)]

    def add_edge(self, u, v, cap):
        self.adj[u].append([v, cap, len(self.adj[v])])
        self.adj[v].append([u, 0, len(self.adj[u]) - 1])

    def _bfs_levels(self, s, t):
        level = [-1] * self.n
        level[s] = 0
        queue = deque([s])
        while queue:
            u = queue.popleft()
            for v, cap, _ in self.adj[u]:
                if cap > 0 and level[v] < 0:
                    level[v] = level[u] + 1
                    queue.append(v)
        return level if level[t] >= 0 else None

    def max_flow(self, s, t):
        flow = 0
        while (level := self._bfs_levels(s, t)) is not None:
            # Iterative blocking-flow DFS with the classic current-arc
            # pointers; the long instances exceed Python's recursion limit.
            # After each augmentation the walk restarts from s -- the pointers
            # survive, so a saturated or dead edge is never scanned twice
            # within a phase.
            it = [0] * self.n
            while True:
                path = [s]
                while path and path[-1] != t:
                    u = path[-1]
                    while it[u] < len(self.adj[u]):
                        v, cap, _ = self.adj[u][it[u]]
                        if cap > 0 and level[v] == level[u] + 1:
                            break
                        it[u] += 1
                    if it[u] == len(self.adj[u]):
                        path.pop()
                        if path:
                            it[path[-1]] += 1
                    else:
                        path.append(self.adj[u][it[u]][0])
                if not path:
                    break
                bottleneck = min(
                    self.adj[path[k]][it[path[k]]][1]
                    for k in range(len(path) - 1)
                )
                for k in range(len(path) - 1):
                    edge = self.adj[path[k]][it[path[k]]]
                    edge[1] -= bottleneck
                    self.adj[edge[0]][edge[2]][1] += bottleneck
                flow += bottleneck
        return flow


def edmonds_karp(n, arcs, s, t):
    """Independent cross-check of the Dinic result."""
    adj = [[] for _ in range(n)]
    for u, v, cap in arcs:
        adj[u].append([v, cap, len(adj[v])])
        adj[v].append([u, 0, len(adj[u]) - 1])
    flow = 0
    while True:
        parent_edge = [None] * n
        parent_edge[s] = (s, -1)
        queue = deque([s])
        while queue and parent_edge[t] is None:
            u = queue.popleft()
            for k, (v, cap, _) in enumerate(adj[u]):
                if cap > 0 and parent_edge[v] is None:
                    parent_edge[v] = (u, k)
                    queue.append(v)
        if parent_edge[t] is None:
            return flow
        bottleneck, v = None, t
        while v != s:
            u, k = parent_edge[v]
            cap = adj[u][k][1]
            bottleneck = cap if bottleneck is None else min(bottleneck, cap)
            v = u
        v = t
        while v != s:
            u, k = parent_edge[v]
            adj[u][k][1] -= bottleneck
            adj[v][adj[u][k][2]][1] += bottleneck
            v = u
        flow += bottleneck


def solve_dinic(n, arcs, s, t):
    dinic = Dinic(n)
    for u, v, cap in arcs:
        dinic.add_edge(u, v, cap)
    return dinic.max_flow(s, t)


def write_instance(out_dir, name, a, b):
    seed = int.from_bytes(hashlib.sha256(name.encode()).digest()[:8], "big")
    n, arcs = build_rmf(a, b, random.Random(seed))
    s, t = 0, n - 1

    flow = solve_dinic(n, arcs, s, t)
    check = edmonds_karp(n, arcs, s, t)
    if flow != check:
        raise AssertionError(
            f"{name}: Dinic found {flow} but Edmonds-Karp found {check} -- "
            "reference solver bug, nothing was written"
        )

    max_path = out_dir / f"{name}.max"
    with open(max_path, "w") as f:
        f.write(f"c RMF (Goldfarb-Grigoriadis) a={a} b={b} c1={C1} c2={C2}\n")
        f.write(f"c generated by scripts/generate_rmf.py, seed sha256('{name}')\n")
        f.write(f"p max {n} {len(arcs)}\n")
        f.write(f"n {s + 1} s\n")
        f.write(f"n {t + 1} t\n")
        for u, v, cap in arcs:
            f.write(f"a {u + 1} {v + 1} {cap}\n")
    with open(out_dir / f"{name}.sol", "w") as f:
        f.write(f"c optimal flow, Dinic + Edmonds-Karp agreement\n")
        f.write(f"s {flow}\n")
    print(f"wrote {max_path}: n={n} m={len(arcs)} flow={flow}")
    return n, len(arcs)


def main():
    out_dir = Path("data/rmf")
    out_dir.mkdir(parents=True, exist_ok=True)

    sizes = {}
    for name, a, b in INSTANCES:
        sizes[name] = write_instance(out_dir, name, a, b)

    header = Path("include/rmf_instances.hpp")
    with open(header, "w") as f:
        f.write(
            "#pragma once\n\n"
            "// Generated by scripts/generate_rmf.py -- do not edit; the\n"
            "// (path, |V|, |A|) triples are written from the actual generated\n"
            "// graphs, so they cannot drift from the data.\n\n"
            "#include <filesystem>\n#include <tuple>\n#include <vector>\n\n"
            "std::vector<std::tuple<std::filesystem::path, int, int>> instances = {\n"
        )
        f.write(",\n".join(
            f'    {{"data/rmf/{name}.max", {n}, {m}}}'
            for name, (n, m) in sizes.items()
        ))
        f.write("};\n")
    print(f"wrote {header}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

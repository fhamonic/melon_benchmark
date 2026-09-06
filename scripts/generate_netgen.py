"""Generate NETGEN-family min-cost-flow instances, reference optima, and the
instance list header.

Usage: python scripts/generate_netgen.py   (from the repository root)

Writes data/netgen/<name>.min (DIMACS min-cost-flow format), a sibling
<name>.sol with the optimal cost, and include/netgen_instances.hpp with the
exact (path, |V|, |A|) of what was generated -- so the header cannot drift
from the data, unlike the hand-maintained instance lists (README, "Known
limitations" 5).

The networks follow the NETGEN structure of Klingman, Napier and Stutz: a set
of source nodes carrying the total supply, a set of sink nodes carrying the
matching demand, transshipment nodes in between, and arcs with random costs of
which a given fraction are capacitated. Feasibility is guaranteed
constructively by a skeleton: a Hamiltonian cycle over a seeded random
permutation of every node, with capacity equal to the total supply. Any
balanced supply vector can be routed around that cycle, so no generated
instance can come out infeasible; skeleton arcs draw their costs from the same
range as every other arc, so they are not merely artificial. This is not an
implementation of the NETGEN generator, but it is the same family.

Everything is deterministic: each instance seeds its own RNG with a hash of
its name.

The optimum is computed here by successive shortest paths with Johnson
potentials, and then *proved* rather than cross-checked against a second
solver: the script verifies that the flow is feasible (capacity bounds and
conservation at every node) and that the final potentials certify it optimal
(every residual arc has non-negative reduced cost). Feasibility plus that dual
certificate is complementary slackness, which is a proof of optimality -- a
stronger guarantee than two independent solvers agreeing.
"""

import hashlib
import heapq
import random
import sys
from pathlib import Path

COST_LO, COST_HI = 1, 1000
# Fraction of the non-skeleton arcs that get a finite capacity; the rest are
# uncapacitated, which in a file with no infinity token means a capacity equal
# to the total supply -- no arc can carry more than that anyway.
CAPACITATED_FRACTION = 0.7

# (name, |V|, |A|, sources, sinks, total supply)
INSTANCES = [
    ("netgen_n1024_m8192", 1024, 8192, 32, 32, 100_000),
    ("netgen_n2048_m32768", 2048, 32768, 64, 64, 200_000),
    ("netgen_n4096_m32768", 4096, 32768, 64, 64, 400_000),
    ("netgen_n8192_m65536", 8192, 65536, 128, 128, 800_000),
]


def split_supply(total, parts, rng):
    """`parts` positive integers summing to exactly `total`."""
    # Random weights normalised to the total rather than independent draws
    # around the even share: independent draws leave a rounding remainder that
    # grows with `parts` and can swamp -- even flip the sign of -- whichever
    # part absorbs it.
    weights = [rng.uniform(0.75, 1.25) for _ in range(parts)]
    scale = total / sum(weights)
    values = [max(1, int(w * scale)) for w in weights]
    values[0] += total - sum(values)
    if min(values) <= 0:
        raise AssertionError("supply split produced a non-positive share")
    return values


def build_netgen(n, m, num_sources, num_sinks, total_supply, rng):
    """Returns (supplies, arcs) with arcs as (u, v, capacity, cost), 0-indexed.

    Sources are the first `num_sources` nodes and sinks the last `num_sinks`,
    so which node plays which role is readable from the file without parsing
    the whole node list.
    """
    supplies = [0] * n
    for i, s in enumerate(split_supply(total_supply, num_sources, rng)):
        supplies[i] = s
    for i, d in enumerate(split_supply(total_supply, num_sinks, rng)):
        supplies[n - 1 - i] = -d

    arcs = []
    seen = set()

    cycle = list(range(n))
    rng.shuffle(cycle)
    for i in range(n):
        u, v = cycle[i], cycle[(i + 1) % n]
        arcs.append((u, v, total_supply, rng.randint(COST_LO, COST_HI)))
        seen.add((u, v))

    cap_lo = max(1, total_supply // (4 * num_sources))
    cap_hi = max(cap_lo + 1, total_supply // num_sources)
    while len(arcs) < m:
        u = rng.randrange(n)
        v = rng.randrange(n)
        # No self-loops and no parallel arcs: both are legal DIMACS and both
        # libraries accept them, but they make |A| a poor description of the
        # instance and a self-loop is dead weight in every solver.
        if u == v or (u, v) in seen:
            continue
        seen.add((u, v))
        capacity = (
            rng.randint(cap_lo, cap_hi)
            if rng.random() < CAPACITATED_FRACTION
            else total_supply
        )
        arcs.append((u, v, capacity, rng.randint(COST_LO, COST_HI)))

    return supplies, arcs


class MinCostFlow:
    """Successive shortest paths with Johnson potentials.

    Residual arcs are held in one flat edge array with paired indices (e ^ 1
    is the reverse of e), so the certificate check below can walk exactly the
    arcs the algorithm reasoned about.
    """

    def __init__(self, n):
        self.n = n
        self.head = []
        self.cap = []
        self.cost = []
        self.adj = [[] for _ in range(n)]

    def add_edge(self, u, v, cap, cost):
        self.adj[u].append(len(self.head))
        self.head.append(v)
        self.cap.append(cap)
        self.cost.append(cost)
        self.adj[v].append(len(self.head))
        self.head.append(u)
        self.cap.append(0)
        self.cost.append(-cost)

    def solve(self, s, t, required):
        """Routes exactly `required` units from s to t at minimum cost.

        Returns (cost, potentials). All costs are non-negative and the initial
        flow is zero, so the zero potential vector is valid to start from and
        the first Dijkstra needs no Bellman-Ford warm-up.
        """
        pi = [0] * self.n
        total_cost = 0
        sent = 0
        INF = float("inf")
        while sent < required:
            dist = [INF] * self.n
            dist[s] = 0
            parent = [-1] * self.n
            done = [False] * self.n
            queue = [(0, s)]
            while queue:
                d, u = heapq.heappop(queue)
                if done[u]:
                    continue
                done[u] = True
                for e in self.adj[u]:
                    if self.cap[e] <= 0:
                        continue
                    v = self.head[e]
                    nd = d + self.cost[e] + pi[u] - pi[v]
                    if nd < dist[v]:
                        dist[v] = nd
                        parent[v] = e
                        heapq.heappush(queue, (nd, v))
            if dist[t] is INF or dist[t] == INF:
                raise AssertionError(
                    "instance is infeasible: the skeleton cycle should have "
                    "made that impossible"
                )
            # Unreachable nodes keep a potential that would leave a residual
            # arc with a negative reduced cost, so they are clamped to the
            # sink's distance -- the standard SSP potential update, and the
            # one the certificate check below depends on.
            for v in range(self.n):
                pi[v] += dist[v] if dist[v] < dist[t] else dist[t]

            bottleneck = required - sent
            v = t
            while v != s:
                e = parent[v]
                bottleneck = min(bottleneck, self.cap[e])
                v = self.head[e ^ 1]
            v = t
            while v != s:
                e = parent[v]
                self.cap[e] -= bottleneck
                self.cap[e ^ 1] += bottleneck
                total_cost += bottleneck * self.cost[e]
                v = self.head[e ^ 1]
            sent += bottleneck
        return total_cost, pi

    def certify(self, pi):
        """Raises unless every residual arc has non-negative reduced cost."""
        for e in range(len(self.head)):
            if self.cap[e] <= 0:
                continue
            u, v = self.head[e ^ 1], self.head[e]
            if self.cost[e] + pi[u] - pi[v] < 0:
                raise AssertionError(
                    f"residual arc {u}->{v} has negative reduced cost: the "
                    "flow is not optimal and nothing was written"
                )


def solve_instance(n, supplies, arcs):
    """(optimal cost, flow per arc) for the balanced transshipment problem.

    A super source and super sink turn the multi-source multi-sink problem
    into the single-pair one the solver handles; their arcs are saturated
    exactly when every supply is shipped and every demand met, so a feasible
    flow of the required value is a feasible solution of the original problem.
    """
    source, sink = n, n + 1
    mcf = MinCostFlow(n + 2)
    for u, v, capacity, cost in arcs:
        mcf.add_edge(u, v, capacity, cost)
    arc_edges = list(range(0, 2 * len(arcs), 2))

    required = 0
    for v, b in enumerate(supplies):
        if b > 0:
            mcf.add_edge(source, v, b, 0)
            required += b
        elif b < 0:
            mcf.add_edge(v, sink, -b, 0)

    cost, pi = mcf.solve(source, sink, required)
    mcf.certify(pi)

    flows = [arcs[i][2] - mcf.cap[e] for i, e in enumerate(arc_edges)]
    check_feasible(n, supplies, arcs, flows)
    if sum(f * c for f, (_, _, _, c) in zip(flows, arcs)) != cost:
        raise AssertionError("cost accumulated during augmentation disagrees "
                             "with the cost of the flow it produced")
    return cost


def check_feasible(n, supplies, arcs, flows):
    """Capacity bounds and conservation, independently of how flows was found."""
    balance = [0] * n
    for (u, v, capacity, _), f in zip(arcs, flows):
        if not 0 <= f <= capacity:
            raise AssertionError(f"flow {f} on arc {u}->{v} violates [0, {capacity}]")
        balance[u] += f
        balance[v] -= f
    for v in range(n):
        if balance[v] != supplies[v]:
            raise AssertionError(
                f"node {v} ships {balance[v]} but its supply is {supplies[v]}"
            )


def write_instance(out_dir, name, n, m, num_sources, num_sinks, total_supply):
    seed = int.from_bytes(hashlib.sha256(name.encode()).digest()[:8], "big")
    rng = random.Random(seed)
    supplies, arcs = build_netgen(n, m, num_sources, num_sinks, total_supply, rng)

    cost = solve_instance(n, supplies, arcs)

    min_path = out_dir / f"{name}.min"
    with open(min_path, "w") as f:
        f.write(
            f"c NETGEN-family min-cost flow: {num_sources} sources, "
            f"{num_sinks} sinks, total supply {total_supply}\n"
        )
        f.write(f"c generated by scripts/generate_netgen.py, seed sha256('{name}')\n")
        f.write(f"p min {n} {len(arcs)}\n")
        for v, b in enumerate(supplies):
            if b != 0:
                f.write(f"n {v + 1} {b}\n")
        for u, v, capacity, arc_cost in arcs:
            f.write(f"a {u + 1} {v + 1} 0 {capacity} {arc_cost}\n")
    with open(out_dir / f"{name}.sol", "w") as f:
        f.write("c optimal cost, successive shortest paths with a verified "
                "dual certificate\n")
        f.write(f"s {cost}\n")
    print(f"wrote {min_path}: n={n} m={len(arcs)} cost={cost}")
    return n, len(arcs)


def main():
    out_dir = Path("data/netgen")
    out_dir.mkdir(parents=True, exist_ok=True)

    sizes = {}
    for name, n, m, num_sources, num_sinks, total_supply in INSTANCES:
        sizes[name] = write_instance(
            out_dir, name, n, m, num_sources, num_sinks, total_supply
        )

    header = Path("include/netgen_instances.hpp")
    with open(header, "w") as f:
        f.write(
            "#pragma once\n\n"
            "// Generated by scripts/generate_netgen.py -- do not edit; the\n"
            "// (path, |V|, |A|) triples are written from the actual generated\n"
            "// graphs, so they cannot drift from the data.\n\n"
            "#include <filesystem>\n#include <tuple>\n#include <vector>\n\n"
            "std::vector<std::tuple<std::filesystem::path, int, int>> instances = {\n"
        )
        f.write(",\n".join(
            f'    {{"data/netgen/{name}.min", {n}, {m}}}'
            for name, (n, m) in sizes.items()
        ))
        f.write("};\n")
    print(f"wrote {header}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

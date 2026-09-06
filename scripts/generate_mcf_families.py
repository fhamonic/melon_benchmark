"""Generate the assignment, transport and circulation min-cost-flow families.

Usage: python scripts/generate_mcf_families.py   (from the repository root)

NETGEN (scripts/generate_netgen.py) is one structure, and it is the one where
the entering-arc block rule likes a scattered arc order. These three families
cover the shapes a network simplex is actually asked for, and they stress
different parts of the implementations:

  assignment  -- k x k complete bipartite, unit supplies and unit capacities.
                 The canonical short solve: k augmentations over k^2 arcs, so
                 the pivot count is tiny next to the arc count and whatever a
                 library spends *setting up* dominates the run.
  transport   -- dense bipartite with real supplies and capacities, no
                 transshipment. The classic OR shape, and long enough that the
                 entering-arc scan dominates instead of the setup.
  circulation -- all supplies zero, a fraction of the arcs carrying negative
                 costs. The optimum is a set of negative cycles. This is also
                 the only family here with negative costs, which is a
                 correctness dimension the other two never touch.

Everything is deterministic: each instance seeds its own RNG with a hash of
its name. Files are DIMACS min-cost-flow format, as for NETGEN.

Reference optima are computed with the same successive-shortest-paths solver
and dual certificate as generate_netgen.py -- feasibility plus non-negative
reduced costs on every residual arc, which is a proof, not a cross-check. It
is O(augmentations x Dijkstra) in Python, so instances above
CERTIFY_MAX_ARCS ship without a .sol; there the benchmarks fall back to the
cross-library digest gate in validate.py, exactly as the SNAP max-flow
instances do.
"""

import hashlib
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_netgen import MinCostFlow, check_feasible

# Above this the Python reference is too slow to be part of `make data`.
CERTIFY_MAX_ARCS = 200_000


def build_assignment(k, rng):
    """k x k complete bipartite, one unit of supply per left node."""
    supplies = [1] * k + [-1] * k
    arcs = [(i, k + j, 1, rng.randint(1, 1000))
            for i in range(k) for j in range(k)]
    return 2 * k, supplies, arcs


def build_transport(num_sources, num_sinks, total_supply, rng):
    """Dense bipartite transportation: every source reaches every sink.

    Supplies and demands are split with the same normalise-then-round scheme
    generate_netgen uses, so they balance exactly. Arc capacities are large
    enough that no single arc is forced, but small enough to bind.
    """
    from generate_netgen import split_supply
    n = num_sources + num_sinks
    supplies = [0] * n
    for i, s in enumerate(split_supply(total_supply, num_sources, rng)):
        supplies[i] = s
    for i, d in enumerate(split_supply(total_supply, num_sinks, rng)):
        supplies[num_sources + i] = -d
    # A source can ship at most this much down any one arc, so the optimum has
    # to spread over several sinks rather than collapsing to a matching.
    cap = max(1, total_supply // (num_sources * 2))
    arcs = [(i, num_sources + j, cap, rng.randint(1, 1000))
            for i in range(num_sources) for j in range(num_sinks)]
    return n, supplies, arcs


def build_circulation(n, m, negative_fraction, rng):
    """Zero supplies everywhere; a fraction of the arcs have negative costs.

    Every arc is capacitated, so the problem is bounded, and the zero flow is
    feasible, so it is solvable. The optimum is strictly negative whenever a
    negative cycle exists, which it does at these densities.
    """
    supplies = [0] * n
    arcs = []
    seen = set()
    while len(arcs) < m:
        u, v = rng.randrange(n), rng.randrange(n)
        if u == v or (u, v) in seen:
            continue
        seen.add((u, v))
        cost = (-rng.randint(1, 1000) if rng.random() < negative_fraction
                else rng.randint(1, 1000))
        arcs.append((u, v, rng.randint(1, 1000), cost))
    return n, supplies, arcs


def solve(n, supplies, arcs):
    """Certified optimum, or None when the instance is too big to certify.

    Negative costs are removed first by the standard reduction: an arc (u, v)
    with cost c < 0 and capacity U is saturated and replaced by (v, u) with
    cost -c and capacity U, moving U units of supply from u to v and adding
    c * U to the objective. A flow g on the replacement means U - g on the
    original, so the two problems correspond one to one and the reduced one
    has non-negative costs, which is what the Dijkstra-based solver needs.
    """
    if len(arcs) > CERTIFY_MAX_ARCS:
        return None

    balance = list(supplies)
    reduced = []
    constant = 0
    for u, v, capacity, cost in arcs:
        if cost < 0:
            constant += cost * capacity
            balance[u] -= capacity
            balance[v] += capacity
            reduced.append((v, u, capacity, -cost))
        else:
            reduced.append((u, v, capacity, cost))

    source, sink = n, n + 1
    mcf = MinCostFlow(n + 2)
    for u, v, capacity, cost in reduced:
        mcf.add_edge(u, v, capacity, cost)
    arc_edges = list(range(0, 2 * len(reduced), 2))

    required = 0
    for v, b in enumerate(balance):
        if b > 0:
            mcf.add_edge(source, v, b, 0)
            required += b
        elif b < 0:
            mcf.add_edge(v, sink, -b, 0)

    cost, pi = mcf.solve(source, sink, required)
    mcf.certify(pi)

    flows = [reduced[i][2] - mcf.cap[e] for i, e in enumerate(arc_edges)]
    check_feasible(n, balance, reduced, flows)
    return constant + cost


# (family, name, builder arguments)
INSTANCES = [
    ("assignment", "assignment_k200", lambda r: build_assignment(200, r)),
    ("assignment", "assignment_k400", lambda r: build_assignment(400, r)),
    ("assignment", "assignment_k700", lambda r: build_assignment(700, r)),
    ("transport", "transport_s256_t256",
     lambda r: build_transport(256, 256, 1_000_000, r)),
    ("transport", "transport_s512_t512",
     lambda r: build_transport(512, 512, 4_000_000, r)),
    ("transport", "transport_s1024_t1024",
     lambda r: build_transport(1024, 1024, 16_000_000, r)),
    # Circulation is by far the most pivot-hungry family -- solve time grows
    # about 4.5x per doubling, where the other three grow roughly linearly --
    # so its sizes are chosen to land in the same tens-of-milliseconds band as
    # the rest of the suite rather than to match their arc counts. A 262144-arc
    # circulation takes 19 s per solve and would dominate `make benchmark` on
    # its own, the same reason the max-flow suite leaves Edmonds-Karp disabled.
    ("circulation", "circulation_n512_m8192",
     lambda r: build_circulation(512, 8192, 0.30, r)),
    ("circulation", "circulation_n1024_m16384",
     lambda r: build_circulation(1024, 16384, 0.30, r)),
    ("circulation", "circulation_n2048_m32768",
     lambda r: build_circulation(2048, 32768, 0.30, r)),
]


def write_instance(family, name, build):
    seed = int.from_bytes(hashlib.sha256(name.encode()).digest()[:8], "big")
    n, supplies, arcs = build(random.Random(seed))

    out_dir = Path("data") / family
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / f"{name}.min"
    with open(path, "w") as f:
        f.write(f"c {family} min-cost flow instance\n")
        f.write(f"c generated by scripts/generate_mcf_families.py,"
                f" seed sha256('{name}')\n")
        f.write(f"p min {n} {len(arcs)}\n")
        for v, b in enumerate(supplies):
            if b != 0:
                f.write(f"n {v + 1} {b}\n")
        for u, v, capacity, cost in arcs:
            f.write(f"a {u + 1} {v + 1} 0 {capacity} {cost}\n")

    optimum = solve(n, supplies, arcs)
    if optimum is not None:
        with open(out_dir / f"{name}.sol", "w") as f:
            f.write("c optimal cost, successive shortest paths with a"
                    " verified dual certificate\n")
            f.write(f"s {optimum}\n")
    print(f"wrote {path}: n={n} m={len(arcs)} "
          f"{'cost=' + str(optimum) if optimum is not None else '(uncertified)'}")
    return n, len(arcs)


def main():
    sizes = {}
    for family, name, build in INSTANCES:
        sizes.setdefault(family, {})[name] = write_instance(family, name, build)

    for family, entries in sizes.items():
        header = Path("include") / f"{family}_instances.hpp"
        with open(header, "w") as f:
            f.write(
                "#pragma once\n\n"
                "// Generated by scripts/generate_mcf_families.py -- do not\n"
                "// edit; the (path, |V|, |A|) triples are written from the\n"
                "// actual generated graphs, so they cannot drift from the\n"
                "// data.\n\n"
                "#include <filesystem>\n#include <tuple>\n#include <vector>\n\n"
                "std::vector<std::tuple<std::filesystem::path, int, int>>"
                " instances = {\n")
            f.write(",\n".join(
                f'    {{"data/{family}/{name}.min", {n}, {m}}}'
                for name, (n, m) in entries.items()))
            f.write("};\n")
        print(f"wrote {header}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

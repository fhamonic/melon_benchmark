"""Assert that every library computed the same answer before anyone plots a bar.

Each benchmark publishes a digest of its result as its Google Benchmark label
(see include/checksum.hpp). For a given algorithm, dataset and instance, every
library, every graph container and every value type solves the identical
problem, so every digest must be identical. A mismatch means at least one
series is not measuring the advertised computation and its timings are
meaningless -- which is exactly how Boost's `double` Dijkstra ran for months on
truncated integer arithmetic.

Usage: python validate.py <results_dir>   # one run directory, results/<run_id>
Exits non-zero if any group disagrees, so `make` stops before plotting.
"""

import json
import os
import re
import sys
from collections import defaultdict

from bench_results import parse_result_filename

# Benchmark names are "<instance>/<container>[:<variant>]/<param>...". Some of
# those params pick an implementation, some change the problem being solved,
# and only the first kind may be collapsed into one validation group.
#
# These are the implementation ones. The value type is here on purpose:
# digests are quantized so `int` and `double` agree, and requiring them to is
# exactly the check that caught Boost's `double` Dijkstra running on truncated
# integer arithmetic. Heap arity likewise cannot change a shortest path.
#
# Everything else -- the k of a k-nearest query, any future task parameter --
# stays in the key. That is the fail-safe direction: an unrecognized param
# splits a group, costing a cross-check, rather than silently comparing runs
# that were never supposed to produce the same answer.
IMPLEMENTATION_PARAMS = (
    re.compile(r"^(int|double|unweighted)$"),  # value type
    re.compile(r"^\d+-heap$"),  # heap arity
)


def split_params(params):
    """(task params that belong in the key, implementation params)"""
    task, implementation = [], []
    for param in params:
        if any(pattern.match(param) for pattern in IMPLEMENTATION_PARAMS):
            implementation.append(param)
        else:
            task.append(param)
    return tuple(task), tuple(implementation)

# Algorithms whose result is not a function of the graph alone.
#
# A traversal forest is built by repeatedly picking *some* unvisited vertex as
# a root and traversing from it, so the partition it produces depends on the
# order roots are chosen in -- and that order is a property of the container,
# not of the graph. MELON's static_digraph and Boost's CSR iterate vertices in
# ascending id order; MELON's mutable_digraph walks an intrusive list and
# LEMON's NodeIt descends, so both pick roots in descending id order. Each
# library is correct; they simply produce different forests.
#
# The digests are still reported, because within one root order they are a
# real cross-library check (MELON's static_digraph and Boost's CSR must agree,
# and they do). They just cannot be required to collapse to a single value.
ORDER_DEPENDENT_ALGORITHMS = {"traversal_forest"}


def load_digests(results_dir):
    """(algorithm, dataset, instance, task params) -> {series -> digest}"""
    digests = defaultdict(dict)
    missing = []

    for filename in sorted(os.listdir(results_dir)):
        if not filename.endswith(".json") or filename.startswith(("_", ".")):
            continue
        library, algorithm, dataset = parse_result_filename(filename)

        try:
            with open(os.path.join(results_dir, filename)) as f:
                data = json.load(f)
        except ValueError as error:
            print(f"warning: skipping unreadable {filename}: {error}", file=sys.stderr)
            continue

        for benchmark in data.get("benchmarks", []):
            name = benchmark["name"]
            # Aggregate rows repeat the label; strip the suffix Google
            # Benchmark appends ("_median", "_stddev", ...).
            run_name = benchmark.get("run_name", name)
            dims = run_name.split("/")
            instance, container = dims[0], dims[1] if len(dims) > 1 else ""
            task, implementation = split_params(dims[2:])
            series = "::".join(
                filter(None, [library, container, "/".join(implementation)])
            )

            label = benchmark.get("label")
            if not label:
                missing.append(f"{filename}: {run_name}")
                continue
            digests[(algorithm, dataset, instance, task)][series] = label

    return digests, sorted(set(missing))


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    results_dir = sys.argv[1]

    digests, missing = load_digests(results_dir)

    disagreements = []
    order_dependent = []
    checked = 0
    for (algorithm, dataset, instance, task), series in sorted(digests.items()):
        distinct = set(series.values())
        checked += 1
        if len(distinct) <= 1:
            continue
        by_digest = defaultdict(list)
        for name, digest in sorted(series.items()):
            by_digest[digest].append(name)
        entry = (algorithm, dataset, instance, task, by_digest)
        if algorithm in ORDER_DEPENDENT_ALGORITHMS:
            order_dependent.append(entry)
        else:
            disagreements.append(entry)

    def report(entries, heading):
        for algorithm, dataset, instance, task, by_digest in entries:
            suffix = (" / " + " / ".join(task)) if task else ""
            print(f"{heading}  {algorithm} / {dataset} / {instance}{suffix}")
            for digest, names in sorted(by_digest.items()):
                print(f"    {digest}")
                for name in names:
                    print(f"        {name}")
            print()

    report(disagreements, "MISMATCH")
    if order_dependent:
        print(
            "The groups below are order-dependent by nature (see "
            "ORDER_DEPENDENT_ALGORITHMS); they are reported, not enforced:\n"
        )
        report(order_dependent, "root-order classes:")

    if missing:
        print(f"warning: {len(missing)} runs published no result digest:")
        for entry in missing[:10]:
            print(f"    {entry}")
        if len(missing) > 10:
            print(f"    ... and {len(missing) - 10} more")
        print()

    if disagreements:
        print(
            f"FAILED: {len(disagreements)} of {checked} "
            f"(algorithm, dataset, instance, task) groups disagree."
        )
        return 1

    print(
        f"OK: all {checked} (algorithm, dataset, instance, task) groups agree "
        "across every library, container and value type"
        + (
            " ({} order-dependent group{} reported above).".format(
                len(order_dependent), "" if len(order_dependent) == 1 else "s"
            )
            if order_dependent
            else "."
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

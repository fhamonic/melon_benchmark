"""Shared parsing of results/ trees.

Two consumers render the same numbers -- plot.py to PNGs and export_web.py to
web/data.json -- and they must never disagree about which Google Benchmark rows
count or how a row becomes milliseconds. This module is the single place those
rules live.

Layout: results/<run_id>/ holds one run -- one (machine, compiler, build
options) combination -- containing <library>-<algorithm>-<dataset>.json files
plus a _provenance.json whose "facets" dict labels the run for humans.
"""

import json
import os
import sys
from collections import defaultdict
from pathlib import Path

TIME_UNITS = {"ns": 1e-6, "us": 1e-3, "ms": 1.0, "s": 1e3}  # -> milliseconds


def parse_result_filename(name):
    """results/<run_id>/<library>-<algorithm>-<dataset>.json"""
    stem = Path(name).stem
    parts = stem.split("-")
    if len(parts) != 3:
        raise ValueError(
            f"cannot parse '{name}': expected <library>-<algorithm>-<dataset>.json. "
            "Library, algorithm and dataset names must not contain '-'."
        )
    return parts


def to_milliseconds(benchmark):
    """Per-item time when the benchmark reports items, wall time otherwise."""
    if "items_per_second" in benchmark:
        return 1000.0 / float(benchmark["items_per_second"])
    return float(benchmark["cpu_time"]) * TIME_UNITS[benchmark["time_unit"]]


def iter_result_files(run_dir):
    """(library, algorithm, dataset, parsed JSON) per readable result file."""
    for filename in sorted(os.listdir(run_dir)):
        if not filename.endswith(".json") or filename.startswith(("_", ".")):
            continue
        library, algorithm, dataset = parse_result_filename(filename)
        try:
            with open(os.path.join(run_dir, filename)) as f:
                data = json.load(f)
        except ValueError as error:
            print(f"warning: skipping unreadable {filename}: {error}", file=sys.stderr)
            continue
        yield library, algorithm, dataset, data


def collect(run_dir):
    """(algorithm, params, dataset) -> series -> instance -> value, twice.

    Returns (medians, stddevs) in milliseconds. With repetitions, Google
    Benchmark emits aggregate rows; the median becomes the bar and the stddev
    the error bar. Without repetitions there is only the single "iteration"
    row and no dispersion.
    """
    medians = defaultdict(lambda: defaultdict(dict))
    stddevs = defaultdict(lambda: defaultdict(dict))

    for library, algorithm, dataset, data in iter_result_files(run_dir):
        for benchmark in data["benchmarks"]:
            run_type = benchmark.get("run_type")
            aggregate = benchmark.get("aggregate_name")

            if run_type == "aggregate" and aggregate not in ("median", "stddev"):
                continue
            if run_type not in ("iteration", "aggregate"):
                continue

            dims = benchmark.get("run_name", benchmark["name"]).split("/")
            instance, graph = dims[0], dims[1]
            params = "|".join(dims[2:])
            series = f"{library}::{graph}"
            key = (algorithm, params, dataset)

            if run_type == "aggregate" and aggregate == "stddev":
                # stddev rows carry the dispersion in the same units; they must
                # not go through items_per_second, which is not additive.
                stddevs[key][series][instance] = (
                    float(benchmark["cpu_time"]) * TIME_UNITS[benchmark["time_unit"]]
                )
            else:
                medians[key][series][instance] = to_milliseconds(benchmark)

    return medians, stddevs


def iter_runs(results_root):
    """(run_id, run_dir, provenance dict) for every run directory.

    A run directory is any direct subdirectory holding a _provenance.json;
    anything else under results_root is ignored, so stray files cannot be
    mistaken for a run.
    """
    for entry in sorted(os.listdir(results_root)):
        run_dir = os.path.join(results_root, entry)
        provenance_path = os.path.join(run_dir, "_provenance.json")
        if not os.path.isdir(run_dir) or not os.path.isfile(provenance_path):
            continue
        try:
            with open(provenance_path) as f:
                provenance = json.load(f)
        except ValueError as error:
            print(
                f"warning: skipping run '{entry}': unreadable _provenance.json: {error}",
                file=sys.stderr,
            )
            continue
        yield entry, run_dir, provenance

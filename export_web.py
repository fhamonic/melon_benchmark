"""Flatten every run under results/ into one compact JSON for the web viewer.

Usage: python export_web.py <results_root> <output.json>

The viewer (web/) is a static ECharts page; it must not know Google
Benchmark's schema or fetch fifty files. This emits a single data.json:

  {
    "schema_version": 1,
    "runs":    { "<run_id>": {"facets": {...}, "timestamp", "git_revision"} },
    "columns": ["run", "algorithm", "params", "dataset", "series", "instance",
                "median_ms", "stddev_ms"],
    "rows":    [ [...], ... ]
  }

Facets ("hardware", "compiler", "options", and whatever future runs add) are
copied verbatim from each run's _provenance.json; the viewer builds one
selector per facet key it finds, so a new dimension is a new provenance key,
not a code change. Rows are arrays, not objects: at ~700 rows per run the
repeated keys would triple the payload for nothing.

A run whose provenance carries no facets is refused rather than exported
unlabeled -- an unlabeled run cannot be selected, only mistaken for another.
"""

import json
import sys

from bench_results import collect, iter_runs

COLUMNS = ["run", "algorithm", "params", "dataset", "series", "instance",
           "median_ms", "stddev_ms"]


def export(results_root):
    runs, rows = {}, []
    for run_id, run_dir, provenance in iter_runs(results_root):
        facets = provenance.get("facets")
        if not facets:
            raise SystemExit(
                f"error: {run_dir}/_provenance.json has no 'facets'; re-run "
                "benchmark.py (or add them by hand for a historical run) -- an "
                "unlabeled run cannot be selected in the viewer."
            )
        runs[run_id] = {
            "facets": facets,
            "timestamp": provenance.get("timestamp"),
            "git_revision": provenance.get("git_revision"),
        }
        medians, stddevs = collect(run_dir)
        for key in sorted(medians):
            algorithm, params, dataset = key
            for series in sorted(medians[key]):
                for instance, median in sorted(medians[key][series].items()):
                    stddev = stddevs.get(key, {}).get(series, {}).get(instance)
                    rows.append([
                        run_id, algorithm, params, dataset, series, instance,
                        # 6 significant digits: well below measurement noise,
                        # and full doubles would double the payload.
                        float(f"{median:.6g}"),
                        float(f"{stddev:.6g}") if stddev is not None else None,
                    ])

    if not runs:
        raise SystemExit(f"error: no run directories under {results_root}")
    return {"schema_version": 1, "runs": runs, "columns": COLUMNS, "rows": rows}


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    results_root, output = sys.argv[1], sys.argv[2]

    data = export(results_root)
    with open(output, "w") as f:
        json.dump(data, f, separators=(",", ":"))
    print(
        f"wrote {output}: {len(data['runs'])} runs, {len(data['rows'])} rows, "
        f"{len(json.dumps(data)) // 1024} KiB"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

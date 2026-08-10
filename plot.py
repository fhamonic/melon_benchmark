"""Render one grouped bar chart per (algorithm, parameters, dataset).

Usage: python plot.py <results_dir> <plots_dir>

<results_dir> is one run directory (results/<run_id>). Bars are the median
over Google Benchmark repetitions, with error bars showing the standard
deviation. A single-repetition run has no dispersion to report, so its bars
are drawn without error bars and the chart says so.

Which rows count and how they become milliseconds lives in bench_results.py,
shared with export_web.py, so the PNGs and the web data cannot drift apart.
"""

import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from bench_results import collect


def plot_one(key, series_data, series_stddev, plots_dir):
    algorithm, params, dataset = key
    series = sorted(series_data.keys())
    if not series:
        return

    instances = sorted(
        {i for s in series for i in series_data[s]},
        key=lambda i: series_data[series[-1]].get(i, 0.0),
    )

    fig, ax = plt.subplots(figsize=(max(10, 1.1 * len(instances) * len(series) / 3), 6))
    plt.rcParams.update({"font.size": 10})

    width = 0.9 / len(series)
    x = np.arange(len(instances))
    have_dispersion = False

    for i, name in enumerate(series):
        values = [series_data[name].get(inst, np.nan) for inst in instances]
        errors = [series_stddev.get(name, {}).get(inst, 0.0) for inst in instances]
        if any(e > 0 for e in errors):
            have_dispersion = True
        offset = -(len(series) - 1) * width / 2 + i * width
        bars = ax.bar(
            x + offset,
            values,
            width,
            label=name,
            yerr=errors if any(e > 0 for e in errors) else None,
            capsize=2,
            error_kw={"elinewidth": 0.8},
        )
        for rect, value, error in zip(bars, values, errors):
            if not np.isfinite(value):
                continue
            ax.annotate(
                "{:.2g}".format(value) if value < 20 else "{}".format(int(value)),
                # Above the error bar, not behind it.
                xy=(rect.get_x() + rect.get_width() / 2, value + error),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center",
                va="bottom",
                fontsize=7,
                rotation=90,
            )

    ax.set_ylabel("milliseconds (median)" if have_dispersion else "milliseconds")

    # Instance runtimes span orders of magnitude on these datasets; on a linear
    # axis the small instances collapse into an unreadable sliver.
    finite = [v for s in series for v in series_data[s].values() if np.isfinite(v) and v > 0]
    if finite and max(finite) / min(finite) > 50:
        ax.set_yscale("log")
        ax.set_ylabel(ax.get_ylabel() + ", log scale")

    title = f"'{algorithm}' runtime on '{dataset}'"
    if params:
        title += f" with parameters '{params}'"
    if not have_dispersion:
        title += "\n(single repetition -- no dispersion measured)"
    ax.set_title(title)
    ax.set_xticks(x)
    ax.set_xticklabels(instances, rotation=60, ha="right")
    ax.legend(fontsize=8)
    ax.margins(y=0.15)

    fig.tight_layout()
    name = f"{algorithm}_{params.replace('|', '_')}_{dataset}" if params else f"{algorithm}_{dataset}"
    # ':' appears in max-flow series names; it is legal in POSIX filenames but
    # confuses enough tools to be worth replacing.
    name = name.replace(":", "-")
    fig.savefig(os.path.join(plots_dir, f"{name}.png"), dpi=120)
    plt.close(fig)
    return name


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    results_dir, plots_dir = sys.argv[1], sys.argv[2]
    os.makedirs(plots_dir, exist_ok=True)

    medians, stddevs = collect(results_dir)
    for key in sorted(medians):
        name = plot_one(key, medians[key], stddevs.get(key, {}), plots_dir)
        if name:
            print(f"wrote {plots_dir}/{name}.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())

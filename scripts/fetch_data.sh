#!/usr/bin/env bash
#
# Fetch the three benchmark datasets into data/.
#
# Roughly 800 MB unpacked. Everything is downloaded from its original
# publisher; nothing is redistributed by this repository.
#
# Usage: scripts/fetch_data.sh [snap|dimacs|bvz|rmf|all]

set -euo pipefail

cd "$(dirname "$0")/.."
DATA_DIR="data"

have() { command -v "$1" >/dev/null 2>&1; }
have curl || { echo "curl is required" >&2; exit 1; }

download() {
    local url="$1" out="$2"
    if [[ -f "$out" ]]; then
        echo "  have $(basename "$out")"
        return
    fi
    echo "  fetching $(basename "$out")"
    curl -fsSL --retry 3 -o "$out.part" "$url"
    mv "$out.part" "$out"
}

# ---------------------------------------------------------------- SNAP -----
# The raw files carry '#' comment headers and are not contiguously indexed.
# The benchmarks expect "<|V|> <|A|>" on the first line, then 0-indexed
# "<from> <to>" pairs, so the ids are remapped here.
fetch_snap() {
    echo "SNAP directed graphs -> $DATA_DIR/snap"
    mkdir -p "$DATA_DIR/snap"
    local base="https://snap.stanford.edu/data"
    for name in web-Stanford Amazon0302 Amazon0505 WikiTalk; do
        local gz="$DATA_DIR/snap/$name.txt.gz"
        local out="$DATA_DIR/snap/$name.txt"
        [[ -f "$out" ]] && { echo "  have $name.txt"; continue; }
        download "$base/$name.txt.gz" "$gz"
        echo "  normalizing $name.txt"
        gunzip -c "$gz" | python3 -c '
import sys

edges, ids = [], {}
for line in sys.stdin:
    if line.startswith("#"):
        continue
    a, b = line.split()[:2]
    for v in (a, b):
        if v not in ids:
            ids[v] = len(ids)
    edges.append((ids[a], ids[b]))

out = sys.stdout
out.write(f"{len(ids)} {len(edges)}\n")
for u, v in edges:
    out.write(f"{u} {v}\n")
' > "$out"
        rm -f "$gz"
        echo "    $(head -1 "$out") vertices/arcs"
    done
    echo
    echo "  NOTE: include/snap_instances.hpp hardcodes |V| and |A| per instance."
    echo "  If the numbers above differ from that file, update it -- they size"
    echo "  the source set and are not cross-checked at runtime."
}

# ------------------------------------------------- 9th DIMACS road networks -
fetch_dimacs() {
    echo "9th DIMACS USA road networks -> $DATA_DIR/9th_DIMACS_USA_roads"
    mkdir -p "$DATA_DIR/9th_DIMACS_USA_roads/distance" \
             "$DATA_DIR/9th_DIMACS_USA_roads/time"
    local base="http://www.diag.uniroma1.it/challenge9/data/USA-road"
    for region in NY BAY COL FLA NW NE; do
        for kind in d t; do
            local dir="distance"; [[ $kind == t ]] && dir="time"
            local out="$DATA_DIR/9th_DIMACS_USA_roads/$dir/USA-road-$kind.$region.gr"
            [[ -f "$out" ]] && { echo "  have $(basename "$out")"; continue; }
            download "$base-$kind/USA-road-$kind.$region.gr.gz" "$out.gz"
            gunzip -f "$out.gz"
        done
    done
}

# ------------------------------------------------------- BVZ-tsukuba max flow
fetch_bvz() {
    echo "BVZ-tsukuba max-flow instances -> $DATA_DIR/BVZ-tsukuba"
    mkdir -p "$DATA_DIR/BVZ-tsukuba"
    local base="https://vision.cs.uwaterloo.ca/files/BVZ-tsukuba.zip"
    local zip="$DATA_DIR/BVZ-tsukuba.zip"
    if [[ -f "$DATA_DIR/BVZ-tsukuba/BVZ-tsukuba0.max" ]]; then
        echo "  have BVZ-tsukuba instances"
        return
    fi
    have unzip || { echo "unzip is required for BVZ-tsukuba" >&2; exit 1; }
    download "$base" "$zip"
    unzip -joq "$zip" -d "$DATA_DIR/BVZ-tsukuba"
    rm -f "$zip"
    echo "  the .sol files are the reference max-flow values; the benchmarks"
    echo "  check against them, so do not delete them"
}

# ----------------------------------------------------- RMF max-flow family
generate_rmf() {
    echo "RMF max-flow instances -> $DATA_DIR/rmf (generated, not downloaded)"
    if [[ -f "$DATA_DIR/rmf/rmf_long_a8_b64.max" ]]; then
        echo "  have RMF instances"
        return
    fi
    python3 scripts/generate_rmf.py
}

case "${1:-all}" in
    snap)   fetch_snap ;;
    dimacs) fetch_dimacs ;;
    bvz)    fetch_bvz ;;
    rmf)    generate_rmf ;;
    all)    fetch_snap; echo; fetch_dimacs; echo; fetch_bvz; echo; generate_rmf ;;
    *)      echo "usage: $0 [snap|dimacs|bvz|rmf|all]" >&2; exit 1 ;;
esac

echo
echo "done. Total size: $(du -sh "$DATA_DIR" 2>/dev/null | cut -f1)"

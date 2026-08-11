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
    # "<local name>:<name SNAP serves it under>". The two differ for three of
    # the four: SNAP's dataset pages are titled Amazon0302 / Amazon0505 /
    # WikiTalk, but the archives are amazon0302 / amazon0505 / wiki-Talk and
    # the titled spellings 404. The local names are the ones
    # include/snap_instances.hpp asks for, so those are what gets written.
    for entry in web-Stanford:web-Stanford Amazon0302:amazon0302 \
                 Amazon0505:amazon0505 WikiTalk:wiki-Talk; do
        local name="${entry%%:*}" remote="${entry##*:}"
        local gz="$DATA_DIR/snap/$name.txt.gz"
        local out="$DATA_DIR/snap/$name.txt"
        [[ -f "$out" ]] && { echo "  have $name.txt"; continue; }
        download "$base/$remote.txt.gz" "$gz"
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
    # rome99 is the smallest instance in include/dimacs_instances.hpp and the
    # first one every *-9th_dimacs benchmark loads, but it is not part of the
    # USA-road family above -- it ships under the challenge's rome/ directory,
    # and only uncompressed: the .gr.gz path there answers with a redirect
    # page, not an archive.
    local rome="$DATA_DIR/9th_DIMACS_USA_roads/rome99.gr"
    download "http://www.diag.uniroma1.it/challenge9/data/rome/rome99.gr" "$rome"
    # That redirect page is served with a 200, so a wrong path does not fail the
    # download -- it lands as an HTML file the benchmarks would abort on. Check
    # for the DIMACS problem line instead.
    if ! grep -q '^p sp ' "$rome"; then
        echo "  rome99.gr is not a DIMACS graph -- removing it" >&2
        rm -f "$rome"
        exit 1
    fi
}

# ------------------------------------------------------- BVZ-tsukuba max flow
fetch_bvz() {
    echo "BVZ-tsukuba max-flow instances -> $DATA_DIR/BVZ-tsukuba"
    mkdir -p "$DATA_DIR/BVZ-tsukuba"
    # Waterloo publishes this as a bzip2 tarball, not a zip; the .zip name 404s.
    # The archive is flat -- 16 .max and their 16 .sol siblings, no directory
    # prefix -- so it extracts straight into the instance directory.
    local base="https://vision.cs.uwaterloo.ca/files/BVZ-tsukuba.tbz2"
    local tarball="$DATA_DIR/BVZ-tsukuba.tbz2"
    if [[ -f "$DATA_DIR/BVZ-tsukuba/BVZ-tsukuba0.max" ]]; then
        echo "  have BVZ-tsukuba instances"
        return
    fi
    have tar || { echo "tar is required for BVZ-tsukuba" >&2; exit 1; }
    download "$base" "$tarball"
    tar -xjf "$tarball" -C "$DATA_DIR/BVZ-tsukuba"
    rm -f "$tarball"
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

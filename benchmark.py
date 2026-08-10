"""Run every benchmark binary and record results plus the provenance of the run.

Usage: python benchmark.py <build_dir> <results_dir> [--repetitions N]
                           [--min-time T] [--filter REGEX] [--force]
                           [--facet KEY=VALUE ...]

<results_dir> holds exactly one run: one (machine, compiler, build options)
combination. The Makefile derives it as results/<host>_<compiler>_<config> and
passes a matching config-keyed <build_dir>, so numbers from different machines,
compilers or option sets can never land in the same directory.

The run's _provenance.json carries a "facets" dict (hardware, compiler,
options, plus any --facet KEY=VALUE) -- the labels downstream selectors group
runs by. Facets not given are derived: hardware from /proc/cpuinfo, compiler
from the compiler the libraries were built with, options from the
OPTIMIZE_FOR_NATIVE entry recorded in each library's CMake cache. A given
options facet is cross-checked against that build evidence and a contradiction
fails the run: a mislabeled facet would silently mislabel every chart derived
from it.

Must be run from the repository root: the binaries resolve dataset paths
relative to the current working directory.
"""

import argparse
import json
import os
import platform
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

# Files that live next to the binaries in the CMake output directory.
NON_BENCHMARK_ENTRIES = {
    "generators",
    "CMakeFiles",
    "CMakeCache.txt",
    "Makefile",
    "metadata",
    "cmake_install.cmake",
    "CTestTestfile.cmake",
    "compile_commands.json",
}


def find_benchmark_binaries(bin_dir, library=None):
    """Executable regular files in bin_dir, excluding CMake's own artifacts.

    Removing a target from a CMakeLists does not delete the executable CMake
    already produced, so a deleted benchmark keeps running and keeps writing
    results for source that is no longer in the tree. Each binary is named
    <algorithm>-<dataset>, so requiring src/<library>/<algorithm>/<dataset>.cpp
    to exist catches exactly that.
    """
    binaries, orphaned = [], []
    for entry in sorted(os.listdir(bin_dir)):
        if entry in NON_BENCHMARK_ENTRIES or entry.startswith("."):
            continue
        path = os.path.join(bin_dir, entry)
        # The original checked os.path.isdir(entry) on the bare name, which is
        # always False, so directories fell through to subprocess.run.
        if not os.path.isfile(path) or not os.access(path, os.X_OK):
            continue
        if library is not None and "-" in entry:
            algorithm, _, dataset = entry.partition("-")
            source = Path("src") / library / algorithm / f"{dataset}.cpp"
            if not source.exists():
                orphaned.append((entry, path))
                continue
        binaries.append(entry)

    for entry, path in orphaned:
        os.remove(path)
        print(f"removed orphaned binary {library}/{entry} (no matching source)")

    return binaries


def result_is_usable(path):
    """True when the file parses and actually contains benchmark rows."""
    try:
        with open(path) as f:
            data = json.load(f)
    except (OSError, ValueError):
        return False
    return bool(data.get("benchmarks"))


CMAKE_TRUTHY = {"ON", "TRUE", "YES", "Y", "1"}


def read_cmake_cache(build_release_dir):
    """The compiler and flags a library was actually built with."""
    cache = os.path.join(build_release_dir, "CMakeCache.txt")
    wanted = {
        "CMAKE_CXX_COMPILER:FILEPATH": "compiler",
        "CMAKE_CXX_FLAGS:STRING": "cxx_flags",
        "CMAKE_CXX_FLAGS_RELEASE:STRING": "cxx_flags_release",
        "CMAKE_BUILD_TYPE:STRING": "build_type",
    }
    found = {}
    try:
        with open(cache) as f:
            for line in f:
                key, _, value = line.partition("=")
                if key in wanted:
                    found[wanted[key]] = value.strip()
                # -march=native is applied via target_compile_options, so it is
                # invisible in CMAKE_CXX_FLAGS*; the OPTIMIZE_FOR_NATIVE cache
                # entry (any type: BOOL via Conan extra_variables, UNINITIALIZED
                # via -D) is the only build-side record that it was on.
                elif key.partition(":")[0] == "OPTIMIZE_FOR_NATIVE":
                    found["optimize_for_native"] = value.strip().upper() in CMAKE_TRUTHY
        # A readable cache with no entry means the option was never set, which
        # is OFF by default -- real evidence, distinct from an unreadable cache.
        found.setdefault("optimize_for_native", False)
    except OSError:
        return found

    compiler = found.get("compiler")
    if compiler:
        try:
            version = subprocess.run(
                [compiler, "--version"], capture_output=True, text=True, timeout=30
            )
            found["compiler_version"] = version.stdout.splitlines()[0].strip()
        except (OSError, subprocess.SubprocessError, IndexError):
            pass
    return found


def read_pinned_dependencies(library):
    """The versions pinned in that library's conanfile -- what was linked."""
    conanfile = Path("src") / library / "conanfile.py"
    try:
        text = conanfile.read_text()
    except OSError:
        return {}
    deps = {}
    for line in text.splitlines():
        if line.lstrip().startswith("#"):  # commented-out requires are not linked
            continue
        match = re.search(r'self\.requires\(\s*["\']([^"\'/]+)/([^"\']+)["\']', line)
        if match:
            deps[match.group(1)] = match.group(2)
    return deps


def hardware_label():
    """A human-facing CPU name; platform.node() is a hostname, not hardware."""
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                key, _, value = line.partition(":")
                if key.strip() == "model name":
                    name = re.sub(r"\((?:R|TM|C)\)", "", value.strip())
                    name = re.sub(r"\s*\d+-Core Processor$", "", name)
                    return re.sub(r"\s+", " ", name).strip()
    except OSError:
        pass
    return platform.processor() or platform.machine()


def compiler_label(library_builds):
    """One label for the toolchain every library was built with.

    The suite's premise is a single compiler across libraries (see README), so
    multiple distinct versions in one build tree is a setup error worth failing
    on -- charting it would attribute codegen differences to the libraries.
    """
    versions = {
        b["build"]["compiler_version"]
        for b in library_builds.values()
        if b["build"].get("compiler_version")
    }
    if len(versions) > 1:
        raise SystemExit(
            "error: libraries were built with different compilers: "
            + ", ".join(sorted(versions))
        )
    if not versions:
        return None
    version = versions.pop()
    match = re.search(r"\(GCC\)\s+([\d.]+)", version)
    if match:
        return f"GCC {match.group(1)}"
    match = re.search(r"clang version\s+([\d.]+)", version)
    if match:
        return f"Clang {match.group(1)}"
    return version


def resolve_options_facet(requested, library_builds):
    """Reconcile the claimed options facet with what the builds record.

    The facet labels every chart derived from this run, so a facet the build
    evidence contradicts is a hard error, not a warning. Missing evidence
    (unreadable cache) only degrades the check, and says so.
    """
    evidence = {
        library: b["build"]["optimize_for_native"]
        for library, b in library_builds.items()
        if "optimize_for_native" in b["build"]
    }
    if evidence and len(set(evidence.values())) > 1:
        raise SystemExit(
            "error: libraries disagree on OPTIMIZE_FOR_NATIVE -- the build tree "
            f"mixes configurations: {evidence}. Rebuild into a clean config dir."
        )
    derived = None
    if evidence:
        derived = "native" if next(iter(evidence.values())) else "generic"

    if requested is None:
        return derived or "generic"
    if derived is not None and requested != derived:
        raise SystemExit(
            f"error: --facet options={requested} but the CMake caches record a "
            f"{derived} build. A mislabeled facet mislabels every chart; fix "
            "the label or rebuild."
        )
    if derived is None:
        print(
            f"warning: no CMake cache recorded OPTIMIZE_FOR_NATIVE; cannot "
            f"cross-check --facet options={requested}",
            file=sys.stderr,
        )
    return requested


def git_revision():
    try:
        out = subprocess.run(
            ["git", "rev-parse", "HEAD"], capture_output=True, text=True, timeout=30
        )
        if out.returncode == 0:
            return out.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        pass
    return None


def read_library_builds(build_dir, libraries):
    return {
        library: {
            "build": read_cmake_cache(os.path.join(build_dir, library, "build/Release")),
            "pinned_dependencies": read_pinned_dependencies(library),
        }
        for library in libraries
    }


def resolve_facets(cli_facets, library_builds):
    """The labels downstream selectors group this run by.

    --facet wins; hardware, compiler and options are otherwise derived, and a
    given options facet is still cross-checked against the build evidence.
    """
    facets = dict(cli_facets)
    facets.setdefault("hardware", hardware_label())
    if "compiler" not in facets:
        label = compiler_label(library_builds)
        if label:
            facets["compiler"] = label
    facets["options"] = resolve_options_facet(facets.get("options"), library_builds)
    return facets


def write_provenance(results_dir, library_builds, facets, args):
    """Record what produced these numbers.

    Without this, a results file six months from now is an unreproducible
    number: nothing else in the repository records the compiler, the flags, or
    the library versions that a given JSON came out of.
    """
    provenance = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "git_revision": git_revision(),
        "host": platform.node(),
        "platform": platform.platform(),
        "processor": platform.processor(),
        "facets": facets,
        "benchmark_args": {
            "repetitions": args.repetitions,
            "min_time": args.min_time,
            "filter": args.filter,
        },
        "libraries": library_builds,
    }

    path = os.path.join(results_dir, "_provenance.json")
    with open(path, "w") as f:
        json.dump(provenance, f, indent=2)
    print(f"wrote {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir")
    parser.add_argument("results_dir")
    parser.add_argument(
        "--repetitions",
        type=int,
        default=10,
        help="Google Benchmark repetitions per case; plot.py needs >1 to draw "
        "error bars (default: 10)",
    )
    parser.add_argument(
        "--min-time",
        default="1s",
        help="Google Benchmark --benchmark_min_time (default: 1s)",
    )
    parser.add_argument("--filter", default=None, help="only run matching benchmarks")
    parser.add_argument(
        "--force", action="store_true", help="re-run even if results look up to date"
    )
    parser.add_argument(
        "--facet",
        action="append",
        default=[],
        metavar="KEY=VALUE",
        help="label this run for downstream selectors (hardware, compiler and "
        "options are derived when not given; repeatable)",
    )
    args = parser.parse_args()

    args.facets = {}
    for facet in args.facet:
        key, sep, value = facet.partition("=")
        if not sep or not key or not value:
            parser.error(f"--facet needs KEY=VALUE, got '{facet}'")
        args.facets[key] = value

    os.makedirs(args.results_dir, exist_ok=True)

    # Two concurrent runs interleave their writes into the same results/ and
    # corrupt each other's files. They also contend for the CPU, which quietly
    # invalidates every timing either one produces.
    lock_path = os.path.join(args.results_dir, ".benchmark.lock")
    try:
        lock_fd = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except FileExistsError:
        print(
            f"error: {lock_path} exists -- another benchmark.py is running, or a\n"
            f"previous one was killed. Timings from a shared machine are not\n"
            f"trustworthy anyway; wait for it, or delete the lock file.",
            file=sys.stderr,
        )
        return 1
    os.write(lock_fd, f"pid {os.getpid()}\n".encode())
    os.close(lock_fd)
    try:
        return run_all(args)
    finally:
        os.unlink(lock_path)


def run_all(args):
    libraries = sorted(
        d for d in os.listdir(args.build_dir)
        if os.path.isdir(os.path.join(args.build_dir, d))
    )

    # Facets are resolved before anything runs: a contradiction between the
    # claimed options facet and the build caches should fail in seconds, not
    # after an hour of benchmarking.
    library_builds = read_library_builds(args.build_dir, libraries)
    facets = resolve_facets(args.facets, library_builds)
    print("run facets: " + ", ".join(f"{k}={v}" for k, v in sorted(facets.items())))

    # Result files whose binary no longer exists are stale: they would still be
    # picked up by validate.py and plot.py and silently charted alongside
    # current numbers.
    expected = set()
    for library in libraries:
        bin_dir = os.path.join(args.build_dir, library, "build/Release")
        if os.path.isdir(bin_dir):
            expected.update(
                f"{library}-{b}.json" for b in find_benchmark_binaries(bin_dir, library)
            )
    for filename in sorted(os.listdir(args.results_dir)):
        if not filename.endswith(".json") or filename.startswith("_"):
            continue
        if filename not in expected:
            os.remove(os.path.join(args.results_dir, filename))
            print(f"removed stale {filename} (no matching binary)")

    failures = []
    for library in libraries:
        bin_dir = os.path.join(args.build_dir, library, "build/Release")
        if not os.path.isdir(bin_dir):
            print(f"warning: {bin_dir} does not exist, skipping {library}")
            continue

        for binary in find_benchmark_binaries(bin_dir):
            exec_path = os.path.join(bin_dir, binary)
            result_path = os.path.join(args.results_dir, f"{library}-{binary}.json")

            if (
                not args.force
                and os.path.exists(result_path)
                and os.path.getmtime(result_path) > os.path.getmtime(exec_path)
            ):
                print(f"up to date: {result_path}")
                continue

            # Google Benchmark writes its JSON incrementally, so a run that is
            # interrupted -- or a second benchmark.py racing the first -- leaves
            # a half-written file behind that every downstream tool then trips
            # over. Write somewhere else and rename only once it parses;
            # os.replace is atomic, so results/ only ever holds whole files.
            partial_path = result_path + ".partial"
            command = [
                exec_path,
                "--benchmark_out_format=json",
                f"--benchmark_out={partial_path}",
                f"--benchmark_repetitions={args.repetitions}",
                f"--benchmark_min_time={args.min_time}",
                # Aggregates only: the per-repetition rows are noise once the
                # median and stddev are recorded, and they triple file size.
                "--benchmark_report_aggregates_only=true",
            ]
            if args.filter:
                command.append(f"--benchmark_filter={args.filter}")

            print(f"running {library}/{binary} ...")
            completed = subprocess.run(command)
            if completed.returncode != 0:
                # A crashed binary used to leave a stale or truncated JSON in
                # place and say nothing about it.
                failures.append(f"{library}/{binary} (exit {completed.returncode})")
                print(
                    f"  FAILED with exit code {completed.returncode}",
                    file=sys.stderr,
                )
                if os.path.exists(partial_path):
                    os.remove(partial_path)
                continue

            # A filter matching nothing makes Google Benchmark write a 0-byte
            # file and exit 0. Left in place it is an unparseable result that
            # breaks validate.py and plot.py.
            if not result_is_usable(partial_path):
                if os.path.exists(partial_path):
                    os.remove(partial_path)
                if args.filter:
                    print("  no benchmarks matched the filter")
                else:
                    failures.append(f"{library}/{binary} (produced no results)")
                    print("  FAILED: produced no usable results", file=sys.stderr)
            else:
                os.replace(partial_path, result_path)

    write_provenance(args.results_dir, library_builds, facets, args)

    if failures:
        print(f"\n{len(failures)} benchmark binaries failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

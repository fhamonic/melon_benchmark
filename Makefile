BUILD_ROOT = build
RESULTS_ROOT = results
PLOTS_ROOT = plots
PYTHON = python3

# All libraries are built with the same compiler; only the language standard
# differs, because MELON requires C++23 and the baselines do not compile as
# C++23. Building them with different compiler *versions* would attribute GCC
# codegen differences to the libraries.
MELON_PROFILE = gcc14_c++23
BASELINE_PROFILE = gcc14_c++20

# NATIVE=ON builds with -march=native (cmake/CompilerOptimizations.cmake).
# Each option set gets its own build/results/plots directories, so flipping it
# can never mix generic and native binaries or numbers: stale-result detection
# in benchmark.py compares mtimes within one (build dir, results dir) pair.
NATIVE ?= OFF
CONFIG = $(if $(filter ON,$(NATIVE)),native,generic)

# A run is one (machine, compiler, build options) combination. The directory
# name is only a key -- the labels selectors display come from the facets in
# results/<run id>/_provenance.json.
COMPILER_TAG = $(firstword $(subst _, ,$(MELON_PROFILE)))
HOST_TAG = $(shell hostname | tr '[:upper:]' '[:lower:]')
RUN_ID = $(HOST_TAG)_$(COMPILER_TAG)_$(CONFIG)

BUILD_DIR = $(BUILD_ROOT)/$(CONFIG)
RESULTS_DIR = $(RESULTS_ROOT)/$(RUN_ID)
PLOTS_DIR = $(PLOTS_ROOT)/$(RUN_ID)

# Passed as a CMake *cache* entry so it is recorded in CMakeCache.txt, where
# benchmark.py cross-checks it against the run's options facet; a plain set()
# would leave no build-side record that -march=native was on.
# Conan eval()s conf values with Python semantics: True, not JSON's true.
NATIVE_CONF = -c 'tools.cmake.cmaketoolchain:extra_variables={"OPTIMIZE_FOR_NATIVE": {"value": "ON", "cache": True, "type": "BOOL"}}'
CONAN_ARGS = -b=missing $(if $(filter ON,$(NATIVE)),$(NATIVE_CONF))

.PHONY: all build benchmark validate plot web data clean

all: plot

# Deliberately not a prerequisite of benchmark: ~800 MB from four third-party
# publishers is not a side effect `make` should have, and none of it is
# redistributable by this repository. An absent dataset is self-explanatory
# anyway -- the benchmarks name the file they could not read and exit 1, which
# benchmark.py reports as a failed binary.
data:
	scripts/fetch_data.sh

build:
	conan build src/melon -of=$(BUILD_DIR)/melon $(CONAN_ARGS) -pr=$(MELON_PROFILE)
	conan build src/lemon -of=$(BUILD_DIR)/lemon $(CONAN_ARGS) -pr=$(BASELINE_PROFILE)
	conan build src/boost -of=$(BUILD_DIR)/boost $(CONAN_ARGS) -pr=$(BASELINE_PROFILE)
	conan build src/cgal -of=$(BUILD_DIR)/cgal $(CONAN_ARGS) -pr=$(BASELINE_PROFILE)
# 	conan build src/cxxgraph -of=$(BUILD_DIR)/cxxgraph $(CONAN_ARGS) -pr=$(BASELINE_PROFILE)

benchmark: build
	$(PYTHON) benchmark.py $(BUILD_DIR) $(RESULTS_DIR) --facet options=$(CONFIG)

# Gates plotting on every library agreeing about the answer. Timings for a
# computation nobody checked are not worth charting.
validate: benchmark
	$(PYTHON) validate.py $(RESULTS_DIR)

plot: validate
	$(PYTHON) plot.py $(RESULTS_DIR) $(PLOTS_DIR)

# Aggregates ALL runs under results/, not just this machine's, into the single
# data.json the web viewer fetches.
web:
	@mkdir -p web
	$(PYTHON) export_web.py $(RESULTS_ROOT) web/data.json

# Only the current configuration's directories: `make clean` on one machine
# must not delete the committed results of every other run.
clean:
	@rm -rf $(BUILD_DIR) $(RESULTS_DIR) $(PLOTS_DIR)

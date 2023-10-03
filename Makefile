MAKEFLAGS += --no-print-directory

CPUS?=$(shell getconf _NPROCESSORS_ONLN || echo 1)
CPU_NAME?=$(shell cat /proc/cpuinfo | grep -i "^model name" | awk -F": " '{print $$2}' | head -1 | sed -E "s/[^A-Za-z0-9]+/_/g" || echo "use_linux_damn_it")

CC = g++-12
MARCH_NATIVE = ON
BUILD_DIR = build
BENCHMARKS_DIR = benchmarks
BENCHMARK_DIR = $(BENCHMARKS_DIR)/$(CPU_NAME)_$(CC)_march-native-${MARCH_NATIVE}
TESTS_DIR = tests

.PHONY: all clean init-submodules update-submodules $(BENCHMARKS)

all: $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
	cmake --build . --config Release --parallel $(CPUS)

$(BUILD_DIR):
	@conan install . -of=$(BUILD_DIR) -b=missing
	@cd $(BUILD_DIR) && \
	cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_CXX_COMPILER=$(CC) -DCMAKE_BUILD_TYPE=Release -DOPTIMIZE_FOR_NATIVE=${MARCH_NATIVE}
clean:
	@rm -rf $(BUILD_DIR)

init-submodules:
	git submodule update --init --recursive
update-submodules:
	git submodule update --recursive --remote

$(BENCHMARKS_DIR):
	@mkdir -p $(BENCHMARKS_DIR)
$(BENCHMARK_DIR): $(BENCHMARKS_DIR)
	@mkdir -p $(BENCHMARK_DIR)

clean-benchmark:
	@rm -rf $(BENCHMARK_DIR)

$(TESTS_DIR):
	@mkdir -p $(TESTS_DIR)

tests-dijkstra: $(TESTS_DIR)
	mkdir -p $(TESTS_DIR)/dijkstra
	./build/dijkstra_tests_bgl > $(TESTS_DIR)/dijkstra/bgl.log
	./build/dijkstra_tests_lemon > $(TESTS_DIR)/dijkstra/lemon.log
	./build/dijkstra_tests_melon > $(TESTS_DIR)/dijkstra/melon.log
	cmp $(TESTS_DIR)/dijkstra/bgl.log $(TESTS_DIR)/dijkstra/lemon.log
	cmp $(TESTS_DIR)/dijkstra/lemon.log $(TESTS_DIR)/dijkstra/melon.log

.SECONDEXPANSION:
$(BENCHMARK_DIR)/%.csv: $(BUILD_DIR)/benchmark_$$(basename $$(subst /,_,$$(subst $$(BENCHMARK_DIR)/,,$$@)))
	@cd $(BENCHMARK_DIR) && \
	mkdir -p $(word 3,$(subst /, ,$@)) && \
	mkdir -p $(word 3,$(subst /, ,$@))/$(word 4,$(subst /, ,$@))
	./$< > $@

BENCHMARKS = benchmark-dijkstra-dimacs-csr_graphs \
benchmark-dijkstra-dimacs-list_graphs \
benchmark-bfs-snap \
benchmark-dfs-snap \
benchmark-dijkstra-dimacs-melon_heap_degree \
benchmark-dijkstra-dimacs-lemon_heap_degree \
benchmark-edmonds_karp-BVZtsukuba

run-benchmarks: $(BENCHMARKS)

benchmark-dijkstra-dimacs-csr_graphs: $(BENCHMARK_DIR) \
$(BENCHMARK_DIR)/dijkstra/dimacs/bgl_compressed_sparse_row.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/lemon_StaticDigraph.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_static_digraph.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_static_digraph_4_heap.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

benchmark-dijkstra-dimacs-list_graphs: $(BENCHMARK_DIR) \
$(BENCHMARK_DIR)/dijkstra/dimacs/bgl_adjacency_list_vecS.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_mutable_digraph.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

benchmark-dijkstra-dimacs-melon_heap_degree: $(BENCHMARK_DIR) \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_static_digraph.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_static_digraph_4_heap.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_static_digraph_8_heap.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/melon_static_digraph_16_heap.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

benchmark-dijkstra-dimacs-lemon_heap_degree: $(BENCHMARK_DIR) \
$(BENCHMARK_DIR)/dijkstra/dimacs/lemon_StaticDigraph.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/lemon_StaticDigraph_4_heap.csv \
$(BENCHMARK_DIR)/dijkstra/dimacs/lemon_StaticDigraph_8_heap.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

benchmark-dijkstra-snap-csr_graphs: $(BENCHMARK_DIR) \
$(BENCHMARK_DIR)/dijkstra/snap/bgl_adjacency_list_vecS.csv \
$(BENCHMARK_DIR)/dijkstra/snap/bgl_compressed_sparse_row.csv \
$(BENCHMARK_DIR)/dijkstra/snap/lemon_StaticDigraph.csv \
$(BENCHMARK_DIR)/dijkstra/snap/melon_static_digraph.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

# benchmark-dijkstra-snap-static_graphs: $(BENCHMARK_DIR) \
# $(BENCHMARK_DIR)/dijkstra/snap/melon_static_digraph.csv \
# $(BENCHMARK_DIR)/dijkstra/snap/melon_static_forward_weighted_digraph.csv
# 	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

benchmark-bfs-snap: $(BENCHMARK_DIR) \
 $(BENCHMARK_DIR)/bfs/snap/bgl_adjacency_list_vecS.csv \
 $(BENCHMARK_DIR)/bfs/snap/bgl_compressed_sparse_row.csv \
 $(BENCHMARK_DIR)/bfs/snap/lemon_StaticDigraph.csv \
 $(BENCHMARK_DIR)/bfs/snap/melon_static_digraph.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

benchmark-dfs-snap: $(BENCHMARK_DIR) \
 $(BENCHMARK_DIR)/dfs/snap/bgl_adjacency_list_vecS.csv \
 $(BENCHMARK_DIR)/dfs/snap/bgl_compressed_sparse_row.csv \
 $(BENCHMARK_DIR)/dfs/snap/lemon_StaticDigraph.csv \
 $(BENCHMARK_DIR)/dfs/snap/melon_static_digraph.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"


benchmark-edmonds_karp-BVZtsukuba: $(BENCHMARK_DIR) \
 $(BENCHMARK_DIR)/edmonds-karp/BVZtsukuba/lemon_StaticDigraph.csv \
 $(BENCHMARK_DIR)/edmonds-karp/BVZtsukuba/melon_static_digraph.csv
	python plot_scripts/execution_times.py "$@" "$(wordlist 2,99,$^)"

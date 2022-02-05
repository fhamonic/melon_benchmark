MAKEFLAGS += --no-print-directory

CPUS?=$(shell getconf _NPROCESSORS_ONLN || echo 1)

BUILD_DIR = build

.PHONY: all clean

all: $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
	cmake --build . --parallel $(CPUS)

$(BUILD_DIR):
	@mkdir $(BUILD_DIR) && \
	cd $(BUILD_DIR) && \
	conan install .. && \
	cmake -DCMAKE_BUILD_TYPE=Release -DWARNINGS=ON -DCOMPILE_FOR_NATIVE=ON -DCOMPILE_WITH_LTO=ON ..

clean:
	@rm -rf $(BUILD_DIR)

dijkstra-tests:
	mkdir -p results/dijkstra/tests/
	./build/bin/dijkstra_tests_bgl > results/dijkstra/tests/bgl.log
	./build/bin/dijkstra_tests_lemon > results/dijkstra/tests/lemon.log
	./build/bin/dijkstra_tests_melon > results/dijkstra/tests/melon.log
	cmp results/dijkstra/tests/bgl.log results/dijkstra/tests/lemon.log
	cmp results/dijkstra/tests/lemon.log results/dijkstra/tests/melon.log

dijkstra-benchmark-dimacs:
	mkdir -p results/dijkstra/dimacs/
	./build/bin/dijkstra_benchmark_dimacs_bgl_adjacency_list_vecS > results/dijkstra/dimacs/bgl_adjacency_list_vecS.csv
	./build/bin/dijkstra_benchmark_dimacs_bgl_compressed_sparse_row > results/dijkstra/dimacs/bgl_compressed_sparse_row.csv
	./build/bin/dijkstra_benchmark_dimacs_lemon_static_graph > results/dijkstra/dimacs/lemon_static_graph.csv
	./build/bin/dijkstra_benchmark_dimacs_melon_static_graph > results/dijkstra/dimacs/melon_static_graph.csv
	python results/dijkstra/dimacs/plot.py

dijkstra-benchmark-snap:
	mkdir -p results/dijkstra/snap/
	./build/bin/dijkstra_benchmark_snap_bgl_adjacency_list_vecS > results/dijkstra/snap/bgl_adjacency_list_vecS.csv
	./build/bin/dijkstra_benchmark_snap_bgl_compressed_sparse_row > results/dijkstra/snap/bgl_compressed_sparse_row.csv
	./build/bin/dijkstra_benchmark_snap_lemon_static_graph > results/dijkstra/snap/lemon_static_graph.csv
	./build/bin/dijkstra_benchmark_snap_melon_static_graph > results/dijkstra/snap/melon_static_graph.csv
	python results/dijkstra/snap/plot.py


bfs-benchmark-snap:
	mkdir -p results/bfs/snap/
	./build/bin/bfs_benchmark_snap_bgl_adjacency_list_vecS > results/bfs/snap/bgl_adjacency_list_vecS.csv
	./build/bin/bfs_benchmark_snap_bgl_compressed_sparse_row > results/bfs/snap/bgl_compressed_sparse_row.csv
	./build/bin/bfs_benchmark_snap_lemon_static_graph > results/bfs/snap/lemon_static_graph.csv
	./build/bin/bfs_benchmark_snap_melon_static_graph > results/bfs/snap/melon_static_graph.csv
	python results/bfs/snap/plot.py

dfs-benchmark-snap:
	mkdir -p results/dfs/snap/
	./build/bin/dfs_benchmark_snap_bgl_adjacency_list_vecS > results/dfs/snap/bgl_adjacency_list_vecS.csv
	./build/bin/dfs_benchmark_snap_bgl_compressed_sparse_row > results/dfs/snap/bgl_compressed_sparse_row.csv
	./build/bin/dfs_benchmark_snap_lemon_static_graph > results/dfs/snap/lemon_static_graph.csv
	./build/bin/dfs_benchmark_snap_melon_static_graph > results/dfs/snap/melon_static_graph.csv
	python results/dfs/snap/plot.py

run-benchmarks: dijkstra-benchmark-dimacs bfs-benchmark-snap dfs-benchmark-snap
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

benchmark-dijkstra:
	mkdir -p results
	mkdir -p results/dijkstra
	./build/bin/dijkstra_benchmark_bgl_adjacency_list_vecS > results/dijkstra/bgl.csv
	./build/bin/dijkstra_benchmark_bgl_compressed_sparse_row > results/dijkstra/bgl_csr.csv
	./build/bin/dijkstra_benchmark_lemon_static_graph > results/dijkstra/lemon_static_graph.csv
	./build/bin/dijkstra_benchmark_melon_static_graph > results/dijkstra/melon_static_graph.csv
	python results/dijkstra/plot.py

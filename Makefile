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

benchmark:
	mkdir -p results
	./build/bin/bgl_csr_benchmark > results/bgl_csr.csv
	./build/bin/bgl_benchmark > results/bgl.csv
	./build/bin/lemon_benchmark > results/lemon.csv
	./build/bin/melon_benchmark > results/melon.csv

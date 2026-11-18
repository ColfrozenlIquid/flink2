BUILD_DIR      := build
ASAN_BUILD_DIR := build-asan
TARGET         := flink2
BENCH_BUILD_DIR := build-bench

.PHONY: all build run debug asan run-asan clean bench

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	@cmake --build $(BUILD_DIR)

run: build
	@./$(BUILD_DIR)/bin/$(TARGET)

debug: build
	@gdb ./$(BUILD_DIR)/bin/$(TARGET)

asan:
	@mkdir -p $(ASAN_BUILD_DIR)
	@cmake -S . -B $(ASAN_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
	@cmake --build $(ASAN_BUILD_DIR)

run-asan: asan
	@./$(ASAN_BUILD_DIR)/bin/$(TARGET)

clean:
	@rm -rf $(BUILD_DIR) $(ASAN_BUILD_DIR)

bench:
	@mkdir -p $(BENCH_BUILD_DIR)
	@cmake -S . -B $(BENCH_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	@cmake --build $(BENCH_BUILD_DIR)
	@./$(BENCH_BUILD_DIR)/bin/$(TARGET)

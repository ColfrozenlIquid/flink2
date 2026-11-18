cmake_minimum_required(VERSION 3.10)
project(Flink2)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS_DEBUG   "-g -O0 -Wall -Wextra -march=native")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -Wall -Wextra")

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O0)
  add_link_options(-fsanitize=address)
endif()

add_executable(flink2
  src/main.c
  src/batch.c
  src/util.c
  src/ops_scalar.c
  src/ops_simd.c
)

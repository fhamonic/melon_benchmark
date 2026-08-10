cmake_minimum_required(VERSION 3.12)

project(
  CGAL_BENCHMARK
  VERSION 1.0
  DESCRIPTION "CGAL benchmark"
  LANGUAGES CXX)

# ################### Options ####################
option(OPTIMIZE_FOR_NATIVE "Build with -march=native" OFF)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../cmake" ${CMAKE_MODULE_PATH})
include(CompilerWarnings)
include(CompilerOptimizations)

# ################### Packages ####################
find_package(benchmark REQUIRED)
find_package(cgal REQUIRED)

# ################## Benchmarks ###################
function(configure_target _target)
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}")
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}/../../include")
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)
  target_link_libraries(${_target} benchmark::benchmark)
  target_link_libraries(${_target} CGAL::CGAL)
  set_project_optimizations(${_target})
  set_project_warnings(${_target})
endfunction()

add_executable(bentley_ottmann-random bentley_ottmann/random.cpp)
configure_target(bentley_ottmann-random)
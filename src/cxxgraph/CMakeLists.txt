cmake_minimum_required(VERSION 3.12)

project(
  CXXGRAPH_BENCHMARK
  VERSION 1.0
  DESCRIPTION "CXXGraph benchmark"
  LANGUAGES CXX)

# ################### Options ####################
option(OPTIMIZE_FOR_NATIVE "Build with -march=native" OFF)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../cmake" ${CMAKE_MODULE_PATH})
include(CompilerWarnings)
include(CompilerOptimizations)

# ################### Packages ####################
find_package(benchmark REQUIRED)
find_package(cxxgraph REQUIRED)

# ################## Benchmarks ###################
function(configure_target _target)
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}")
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}/../../include")
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)
  target_link_libraries(${_target} benchmark::benchmark)
  target_link_libraries(${_target} cxxgraph::cxxgraph)
  set_project_optimizations(${_target})
  set_project_warnings(${_target})
endfunction()

# Only the traversals. CXXGraph's dijkstra is point-to-point -- it returns a
# single distance for one source/target pair -- so it does not answer the
# single-source question the dijkstra benchmark asks and cannot share its
# chart. breadth_first_search and depth_first_search return the reached set,
# which is exactly what the other three libraries are measured on.
add_executable(breadth_first_search-9th_dimacs breadth_first_search/9th_dimacs.cpp)
configure_target(breadth_first_search-9th_dimacs)

add_executable(depth_first_search-9th_dimacs depth_first_search/9th_dimacs.cpp)
configure_target(depth_first_search-9th_dimacs)

# The one shortest-path question CXXGraph's API can answer.
add_executable(dijkstra_point_to_point-9th_dimacs dijkstra_point_to_point/9th_dimacs.cpp)
configure_target(dijkstra_point_to_point-9th_dimacs)

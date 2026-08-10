cmake_minimum_required(VERSION 3.12)

project(
  BOOST_BENCHMARK
  VERSION 1.0
  DESCRIPTION "boost benchmark"
  LANGUAGES CXX)

# ################### Options ####################
option(OPTIMIZE_FOR_NATIVE "Build with -march=native" OFF)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../cmake" ${CMAKE_MODULE_PATH})
include(CompilerWarnings)
include(CompilerOptimizations)

# ################### Packages ####################
find_package(benchmark REQUIRED)
find_package(Boost REQUIRED)

# ################## Benchmarks ###################
function(configure_target _target)
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}")
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}/../../include")
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)
  target_link_libraries(${_target} benchmark::benchmark)
  target_link_libraries(${_target} boost::boost)
  set_project_optimizations(${_target})
  set_project_warnings(${_target})
endfunction()

add_executable(dijkstra-9th_dimacs dijkstra/9th_dimacs.cpp)
configure_target(dijkstra-9th_dimacs)

# Storage-policy comparison across query sizes: k-nearest-vertices search,
# distances consumed online, each library in its storing and its leanest
# configuration.
add_executable(dijkstra_bounded-9th_dimacs dijkstra_bounded/9th_dimacs.cpp)
configure_target(dijkstra_bounded-9th_dimacs)
add_executable(dijkstra_point_to_point-9th_dimacs dijkstra_point_to_point/9th_dimacs.cpp)
configure_target(dijkstra_point_to_point-9th_dimacs)


add_executable(breadth_first_search-snap breadth_first_search/snap.cpp)
configure_target(breadth_first_search-snap)
add_executable(breadth_first_search-9th_dimacs breadth_first_search/9th_dimacs.cpp)
configure_target(breadth_first_search-9th_dimacs)

add_executable(depth_first_search-snap depth_first_search/snap.cpp)
configure_target(depth_first_search-snap)
add_executable(depth_first_search-9th_dimacs depth_first_search/9th_dimacs.cpp)
configure_target(depth_first_search-9th_dimacs)

add_executable(strongly_connected_components-snap strongly_connected_components/snap.cpp)
configure_target(strongly_connected_components-snap)
add_executable(strongly_connected_components-9th_dimacs strongly_connected_components/9th_dimacs.cpp)
configure_target(strongly_connected_components-9th_dimacs)

# # proof that boost::connected_components is wrong on directed graphs
# add_executable(weakly_connected_components-test weakly_connected_components/test.cpp)
# configure_target(weakly_connected_components-test)

add_executable(traversal_forest-snap traversal_forest/snap.cpp)
configure_target(traversal_forest-snap)
add_executable(traversal_forest-9th_dimacs traversal_forest/9th_dimacs.cpp)
configure_target(traversal_forest-9th_dimacs)

# push_relabel / boykov_kolmogorov / edmonds_karp, registered under names of
# the form "<instance>/<container>:<algorithm>/<value>" so that they share a
# chart with MELON's Dinitz and LEMON's Preflow.
add_executable(max_flow-bvz_tsukuba max_flow/bvz_tsukuba.cpp)
configure_target(max_flow-bvz_tsukuba)
add_executable(max_flow-rmf max_flow/rmf.cpp)
configure_target(max_flow-rmf)

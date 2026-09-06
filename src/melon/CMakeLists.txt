cmake_minimum_required(VERSION 3.12)

project(
  MELON_BENCHMARK
  VERSION 1.0
  DESCRIPTION "MELON benchmark"
  LANGUAGES CXX)

# ################### Options ####################
option(OPTIMIZE_FOR_NATIVE "Build with -march=native" OFF)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../cmake" ${CMAKE_MODULE_PATH})
include(CompilerWarnings)
include(CompilerOptimizations)

# ################### Packages ####################
find_package(benchmark REQUIRED)
find_package(melon REQUIRED)

# ################## Benchmarks ###################
function(configure_target _target)
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}")
  target_include_directories(${_target} PUBLIC "${PROJECT_SOURCE_DIR}/../../include")
  # set_target_properties(${_target} PROPERTIES CXX_STANDARD 26)
  target_link_libraries(${_target} benchmark::benchmark)
  target_link_libraries(${_target} melon::melon)
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

add_executable(bellman_ford-9th_dimacs bellman_ford/9th_dimacs.cpp)
configure_target(bellman_ford-9th_dimacs)
add_executable(bellman_ford_moore-9th_dimacs bellman_ford_moore/9th_dimacs.cpp)
configure_target(bellman_ford_moore-9th_dimacs)

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

add_executable(weakly_connected_components-snap weakly_connected_components/snap.cpp)
configure_target(weakly_connected_components-snap)
add_executable(weakly_connected_components-9th_dimacs weakly_connected_components/9th_dimacs.cpp)
configure_target(weakly_connected_components-9th_dimacs)

add_executable(traversal_forest-snap traversal_forest/snap.cpp)
configure_target(traversal_forest-snap)
add_executable(traversal_forest-9th_dimacs traversal_forest/9th_dimacs.cpp)
configure_target(traversal_forest-9th_dimacs)




# Dinitz and Edmonds-Karp are registered by one binary each, under names of
# the form "<instance>/<container>:<algorithm>/<value>", so that they share a
# chart with LEMON's and Boost's max-flow algorithms.
add_executable(max_flow-bvz_tsukuba max_flow/bvz_tsukuba.cpp)
configure_target(max_flow-bvz_tsukuba)
add_executable(max_flow-snap max_flow/snap.cpp)
configure_target(max_flow-snap)
add_executable(max_flow-rmf max_flow/rmf.cpp)
configure_target(max_flow-rmf)

# Min-cost flow on four instance families -- NETGEN, assignment,
# transportation and circulation -- because the families disagree about
# which library wins and one of them alone is not a result. Registered
# under
# "<instance>/<container>:network_simplex/<value>" so that MELON's network
# simplex shares a chart with LEMON's.
add_executable(network_simplex-netgen network_simplex/netgen.cpp)
configure_target(network_simplex-netgen)
add_executable(network_simplex-assignment network_simplex/assignment.cpp)
configure_target(network_simplex-assignment)
add_executable(network_simplex-transport network_simplex/transport.cpp)
configure_target(network_simplex-transport)
add_executable(network_simplex-circulation network_simplex/circulation.cpp)
configure_target(network_simplex-circulation)

add_executable(kruskal-bvz_tsukuba kruskal/bvz_tsukuba.cpp)
configure_target(kruskal-bvz_tsukuba)
add_executable(kruskal-9th_dimacs kruskal/9th_dimacs.cpp)
configure_target(kruskal-9th_dimacs)

add_executable(bentley_ottmann-random bentley_ottmann/random.cpp)
configure_target(bentley_ottmann-random)

# static_filter_map against static_map<K, bool> and std::vector<bool>:
# random access (the visited-map pattern, both cache regimes) and set-key
# enumeration across densities (filter()'s bit scan vs naive scans).
add_executable(static_filter_map-synthetic static_filter_map/synthetic.cpp)
configure_target(static_filter_map-synthetic)

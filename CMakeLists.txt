cmake_minimum_required(VERSION 3.12)

project(
  MEBENCHMARK_LONS
  VERSION 1.0
  DESCRIPTION "Graph libraries benchmarks"
  LANGUAGES CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# ################### Options ####################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WARNINGS "Enable warnings" OFF)
option(OPTIMIZE_FOR_NATIVE "Build with -march=native" OFF)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(GNUInstallDirs)
include(CompilerWarnings)
include(CompilerOptimizations)
include(Conan)
run_conan()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}" ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/dependencies/melon"
                      ${CMAKE_MODULE_PATH})

add_subdirectory(dependencies/melon)

# ################### Packages ####################
find_package(Boost REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LEMON REQUIRED)

# ############ COMPILATION OPTIONS MACRO #############
function(set_common_options _target)
  target_include_directories(${_target} PUBLIC include)
  set_project_optimizations(${_target})
  if(WARNINGS)
    set_project_warnings(${_target})
  endif()
endfunction()

function(set_boost_options _target)
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)
  target_link_libraries(${_target} Boost::boost)
  set_common_options(${_target})
endfunction()

function(set_lemon_options _target)
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 17)
  target_include_directories(${_target} PUBLIC ${LEMON_INCLUDE_DIRS})
  target_link_libraries(${_target} ${LEMON_LIBRARIES})
  set_common_options(${_target})
endfunction()

function(set_melon_options _target)
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)
  target_link_libraries(${_target} melon)
  set_common_options(${_target})
endfunction()

function(set_cxxgraph_options _target)
  set_target_properties(${_target} PROPERTIES CXX_STANDARD 17)
  target_include_directories(${_target} PUBLIC dependencies/CXXGraph/include)
  target_link_libraries(${_target} ZLIB::ZLIB)
  target_link_libraries(${_target} OpenSSL::OpenSSL)
  set_common_options(${_target})
endfunction()

# ############### CORRECTNESS TESTS ##################

add_executable(dijkstra_tests_lemon src/tests/dijkstra/lemon.cpp)
set_lemon_options(dijkstra_tests_lemon)

add_executable(dijkstra_tests_bgl src/tests/dijkstra/bgl.cpp)
set_boost_options(dijkstra_tests_bgl)

add_executable(dijkstra_tests_melon src/tests/dijkstra/melon.cpp)
set_melon_options(dijkstra_tests_melon)

# ################### BENCHMARKS #####################

# ######### DIJKSTRA ###########

add_executable(benchmark_dijkstra_dimacs_lemon_StaticDigraph
               src/benchmarks/dijkstra/dimacs/lemon_StaticDigraph.cpp)
set_lemon_options(benchmark_dijkstra_dimacs_lemon_StaticDigraph)
add_executable(benchmark_dijkstra_dimacs_lemon_StaticDigraph_4_heap
               src/benchmarks/dijkstra/dimacs/lemon_StaticDigraph_4_heap.cpp)
set_lemon_options(benchmark_dijkstra_dimacs_lemon_StaticDigraph_4_heap)
add_executable(benchmark_dijkstra_dimacs_lemon_StaticDigraph_8_heap
               src/benchmarks/dijkstra/dimacs/lemon_StaticDigraph_8_heap.cpp)
set_lemon_options(benchmark_dijkstra_dimacs_lemon_StaticDigraph_8_heap)
add_executable(benchmark_dijkstra_dimacs_bgl_adjacency_list_vecS
               src/benchmarks/dijkstra/dimacs/bgl_adjacency_list_vecS.cpp)
set_boost_options(benchmark_dijkstra_dimacs_bgl_adjacency_list_vecS)
add_executable(benchmark_dijkstra_dimacs_bgl_compressed_sparse_row
               src/benchmarks/dijkstra/dimacs/bgl_compressed_sparse_row.cpp)
set_boost_options(benchmark_dijkstra_dimacs_bgl_compressed_sparse_row)
# add_executable(benchmark_dijkstra_dimacs_cxxgraph
# src/benchmarks/dijkstra/dimacs/cxxgraph.cpp)
# set_cxxgraph_options(benchmark_dijkstra_dimacs_cxxgraph)
add_executable(benchmark_dijkstra_dimacs_melon_static_digraph
src/benchmarks/dijkstra/dimacs/melon_static_digraph.cpp)
set_melon_options(benchmark_dijkstra_dimacs_melon_static_digraph)
add_executable(benchmark_dijkstra_dimacs_melon_static_digraph_4_heap
               src/benchmarks/dijkstra/dimacs/melon_static_digraph_4_heap.cpp)
set_melon_options(benchmark_dijkstra_dimacs_melon_static_digraph_4_heap)
add_executable(benchmark_dijkstra_dimacs_melon_static_digraph_8_heap
               src/benchmarks/dijkstra/dimacs/melon_static_digraph_8_heap.cpp)
set_melon_options(benchmark_dijkstra_dimacs_melon_static_digraph_8_heap)
add_executable(benchmark_dijkstra_dimacs_melon_static_digraph_16_heap
               src/benchmarks/dijkstra/dimacs/melon_static_digraph_16_heap.cpp)
set_melon_options(benchmark_dijkstra_dimacs_melon_static_digraph_16_heap)

add_executable(benchmark_dijkstra_snap_lemon_StaticDigraph
               src/benchmarks/dijkstra/snap/lemon_StaticDigraph.cpp)
set_lemon_options(benchmark_dijkstra_snap_lemon_StaticDigraph)
add_executable(benchmark_dijkstra_snap_bgl_adjacency_list_vecS
               src/benchmarks/dijkstra/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(benchmark_dijkstra_snap_bgl_adjacency_list_vecS)
add_executable(benchmark_dijkstra_snap_bgl_compressed_sparse_row
               src/benchmarks/dijkstra/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(benchmark_dijkstra_snap_bgl_compressed_sparse_row)
add_executable(benchmark_dijkstra_snap_melon_static_digraph
               src/benchmarks/dijkstra/snap/melon_static_digraph.cpp)
set_melon_options(benchmark_dijkstra_snap_melon_static_digraph)

# ######### BFS ###########

add_executable(benchmark_bfs_snap_lemon_StaticDigraph
               src/benchmarks/bfs/snap/lemon_StaticDigraph.cpp)
set_lemon_options(benchmark_bfs_snap_lemon_StaticDigraph)
add_executable(benchmark_bfs_snap_bgl_adjacency_list_vecS
               src/benchmarks/bfs/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(benchmark_bfs_snap_bgl_adjacency_list_vecS)
add_executable(benchmark_bfs_snap_bgl_compressed_sparse_row
               src/benchmarks/bfs/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(benchmark_bfs_snap_bgl_compressed_sparse_row)
add_executable(benchmark_bfs_snap_melon_static_digraph
               src/benchmarks/bfs/snap/melon_static_digraph.cpp)
set_melon_options(benchmark_bfs_snap_melon_static_digraph)

# ######### DFS ###########

add_executable(benchmark_dfs_snap_lemon_StaticDigraph
               src/benchmarks/dfs/snap/lemon_StaticDigraph.cpp)
set_lemon_options(benchmark_dfs_snap_lemon_StaticDigraph)
add_executable(benchmark_dfs_snap_bgl_adjacency_list_vecS
               src/benchmarks/dfs/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(benchmark_dfs_snap_bgl_adjacency_list_vecS)
add_executable(benchmark_dfs_snap_bgl_compressed_sparse_row
               src/benchmarks/dfs/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(benchmark_dfs_snap_bgl_compressed_sparse_row)
add_executable(benchmark_dfs_snap_melon_static_digraph
               src/benchmarks/dfs/snap/melon_static_digraph.cpp)
set_melon_options(benchmark_dfs_snap_melon_static_digraph)

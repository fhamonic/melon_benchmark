cmake_minimum_required(VERSION 3.12)

project(
    MELON_BENCHMARKS
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
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/dependencies/melon" ${CMAKE_MODULE_PATH})

add_subdirectory(dependencies/melon)

# ################### Packages ####################
find_package(range-v3 REQUIRED)
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

# ################ CORRECTNESS TEST ##################

add_executable(dijkstra_tests_lemon src/dijkstra/tests/lemon.cpp)
set_lemon_options(dijkstra_tests_lemon)

add_executable(dijkstra_tests_bgl src/dijkstra/tests/bgl.cpp)
set_boost_options(dijkstra_tests_bgl)

add_executable(dijkstra_tests_melon src/dijkstra/tests/melon.cpp)
set_melon_options(dijkstra_tests_melon)

# ################### BENCHMARKS #####################

# ######### DIJKSTRA ###########

add_executable(dijkstra_benchmark_dimacs_lemon_static_digraph
               src/dijkstra/benchmark/dimacs/lemon_static_digraph.cpp)
set_lemon_options(dijkstra_benchmark_dimacs_lemon_static_digraph)
add_executable(dijkstra_benchmark_dimacs_bgl_adjacency_list_vecS
               src/dijkstra/benchmark/dimacs/bgl_adjacency_list_vecS.cpp)
set_boost_options(dijkstra_benchmark_dimacs_bgl_adjacency_list_vecS)
add_executable(dijkstra_benchmark_dimacs_bgl_compressed_sparse_row
               src/dijkstra/benchmark/dimacs/bgl_compressed_sparse_row.cpp)
set_boost_options(dijkstra_benchmark_dimacs_bgl_compressed_sparse_row)
# add_executable(dijkstra_benchmark_dimacs_cxxgraph
#                src/dijkstra/benchmark/dimacs/cxxgraph.cpp)
# set_cxxgraph_options(dijkstra_benchmark_dimacs_cxxgraph)
add_executable(dijkstra_benchmark_dimacs_melon_static_digraph
               src/dijkstra/benchmark/dimacs/melon_static_digraph.cpp)
set_melon_options(dijkstra_benchmark_dimacs_melon_static_digraph)

add_executable(dijkstra_benchmark_snap_lemon_static_digraph
               src/dijkstra/benchmark/snap/lemon_static_digraph.cpp)
set_lemon_options(dijkstra_benchmark_snap_lemon_static_digraph)
add_executable(dijkstra_benchmark_snap_bgl_adjacency_list_vecS
               src/dijkstra/benchmark/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(dijkstra_benchmark_snap_bgl_adjacency_list_vecS)
add_executable(dijkstra_benchmark_snap_bgl_compressed_sparse_row
               src/dijkstra/benchmark/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(dijkstra_benchmark_snap_bgl_compressed_sparse_row)
add_executable(dijkstra_benchmark_snap_melon_static_digraph
               src/dijkstra/benchmark/snap/melon_static_digraph.cpp)
set_melon_options(dijkstra_benchmark_snap_melon_static_digraph)

# ######### BFS ###########

add_executable(bfs_benchmark_snap_lemon_static_digraph
               src/bfs/benchmark/snap/lemon_static_digraph.cpp)
set_lemon_options(bfs_benchmark_snap_lemon_static_digraph)
add_executable(bfs_benchmark_snap_bgl_adjacency_list_vecS
               src/bfs/benchmark/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(bfs_benchmark_snap_bgl_adjacency_list_vecS)
add_executable(bfs_benchmark_snap_bgl_compressed_sparse_row
               src/bfs/benchmark/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(bfs_benchmark_snap_bgl_compressed_sparse_row)
add_executable(bfs_benchmark_snap_melon_static_digraph
               src/bfs/benchmark/snap/melon_static_digraph.cpp)
set_melon_options(bfs_benchmark_snap_melon_static_digraph)

# ######### DFS ###########

add_executable(dfs_benchmark_snap_lemon_static_digraph
               src/dfs/benchmark/snap/lemon_static_digraph.cpp)
set_lemon_options(dfs_benchmark_snap_lemon_static_digraph)
add_executable(dfs_benchmark_snap_bgl_adjacency_list_vecS
               src/dfs/benchmark/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(dfs_benchmark_snap_bgl_adjacency_list_vecS)
add_executable(dfs_benchmark_snap_bgl_compressed_sparse_row
               src/dfs/benchmark/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(dfs_benchmark_snap_bgl_compressed_sparse_row)
add_executable(dfs_benchmark_snap_melon_static_digraph
               src/dfs/benchmark/snap/melon_static_digraph.cpp)
set_melon_options(dfs_benchmark_snap_melon_static_digraph)

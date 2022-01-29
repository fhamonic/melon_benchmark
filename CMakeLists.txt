cmake_minimum_required(VERSION 3.12)

project(
    dijkstra_benchmark_dimacs_melon
    VERSION 1.0
    DESCRIPTION
        "Benchamrking different Dijkstras."
    LANGUAGES CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# ################### Options ####################
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CONAN_DISABLE_CHECK_COMPILER ON)

option(WARNINGS "" OFF)
option(HARDCORE_WARNINGS "" OFF)
option(COMPILE_FOR_NATIVE "" OFF)
option(COMPILE_WITH_LTO "" OFF)

set(WARNINGS_LIST "-Wall;-Wextra;")
set(HARDCORE_WARNINGS_LIST
    "-Wall;-Wextra;-Wshadow;-Wpedantic;-Wnon-virtual-dtor;-Wold-style-cast;-Wcast-align;-Wunused;-Woverloaded-virtual;-Wpedantic;-Wconversion;-Wsign-conversion;-Wmisleading-indentation;-Wduplicated-cond;-Wduplicated-branches;-Wlogical-op;-Wnull-dereference;-Wuseless-cast;-Wdouble-promotion;-Wformat=2;-Wlifetime"
)

# ################### Modules ####################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(GNUInstallDirs)
include(CMakeBuildTypeUtils)

# ################### Packages ####################
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(LEMON REQUIRED)

# ############ COMPILATION OPTIONS MACRO #############
function(set_common_options _target)
target_include_directories(${_target} PUBLIC include)
target_compile_options_if_exists(${_target} PRIVATE ${WARNINGS}
                                 ${WARNINGS_LIST})
target_compile_options_if_exists(${_target} PRIVATE ${COMPILE_FOR_NATIVE}
                                 -march=native)
target_compile_options_if_exists(${_target} PRIVATE ${COMPILE_WITH_LTO}
                                 -flto)
endfunction()

function(set_boost_options _target)
set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${_target} CONAN_PKG::boost)
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
target_link_libraries(${_target} CONAN_PKG::range-v3)
target_link_libraries(${_target} CONAN_PKG::andreasbuhr-cppcoro)
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

add_executable(dijkstra_benchmark_dimacs_lemon_static_graph src/dijkstra/benchmark/dimacs/lemon_static_graph.cpp)
set_lemon_options(dijkstra_benchmark_dimacs_lemon_static_graph)                       
add_executable(dijkstra_benchmark_dimacs_bgl_adjacency_list_vecS src/dijkstra/benchmark/dimacs/bgl_adjacency_list_vecS.cpp)
set_boost_options(dijkstra_benchmark_dimacs_bgl_adjacency_list_vecS)
add_executable(dijkstra_benchmark_dimacs_bgl_compressed_sparse_row src/dijkstra/benchmark/dimacs/bgl_compressed_sparse_row.cpp)
set_boost_options(dijkstra_benchmark_dimacs_bgl_compressed_sparse_row)                                                                                    
add_executable(dijkstra_benchmark_dimacs_melon_static_graph src/dijkstra/benchmark/dimacs/melon_static_graph.cpp)
set_melon_options(dijkstra_benchmark_dimacs_melon_static_graph)

add_executable(dijkstra_benchmark_snap_lemon_static_graph src/dijkstra/benchmark/snap/lemon_static_graph.cpp)
set_lemon_options(dijkstra_benchmark_snap_lemon_static_graph)                       
add_executable(dijkstra_benchmark_snap_bgl_adjacency_list_vecS src/dijkstra/benchmark/snap/bgl_adjacency_list_vecS.cpp)
set_boost_options(dijkstra_benchmark_snap_bgl_adjacency_list_vecS)
add_executable(dijkstra_benchmark_snap_bgl_compressed_sparse_row src/dijkstra/benchmark/snap/bgl_compressed_sparse_row.cpp)
set_boost_options(dijkstra_benchmark_snap_bgl_compressed_sparse_row)                                                                                    
add_executable(dijkstra_benchmark_snap_melon_static_graph src/dijkstra/benchmark/snap/melon_static_graph.cpp)
set_melon_options(dijkstra_benchmark_snap_melon_static_graph)

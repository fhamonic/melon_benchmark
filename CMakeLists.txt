cmake_minimum_required(VERSION 3.12)

project(
    melon_benchmark
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

# ################ CORRECTNESS TEST ##################

# ################## LEMON target ####################
add_executable(lemon_correctness src/correctness_lemon.cpp)
set_target_properties(lemon_correctness PROPERTIES CXX_STANDARD 17)
target_include_directories(lemon_correctness PUBLIC ${LEMON_INCLUDE_DIRS})
target_link_libraries(lemon_correctness ${LEMON_LIBRARIES})
set_common_options(lemon_correctness)
# ################## BOOST target ####################              
add_executable(bgl_correctness src/correctness_bgl.cpp)
set_target_properties(bgl_correctness PROPERTIES CXX_STANDARD 20)
target_link_libraries(bgl_correctness CONAN_PKG::boost)
set_common_options(bgl_correctness)
# ################## MELON target ####################
add_executable(melon_correctness src/correctness_melon.cpp)
set_target_properties(melon_correctness PROPERTIES CXX_STANDARD 20)
target_link_libraries(melon_correctness CONAN_PKG::range-v3)
set_common_options(melon_correctness)

# ################### BENCHMARKS #####################

# ################## LEMON target ####################
add_executable(lemon_benchmark src/benchmark_lemon.cpp)
set_target_properties(lemon_benchmark PROPERTIES CXX_STANDARD 17)
target_include_directories(lemon_benchmark PUBLIC ${LEMON_INCLUDE_DIRS})
target_link_libraries(lemon_benchmark ${LEMON_LIBRARIES})
set_common_options(lemon_benchmark)
# ################## BOOST target ####################                            
add_executable(bgl_benchmark src/benchmark_bgl.cpp)
set_target_properties(bgl_benchmark PROPERTIES CXX_STANDARD 20)
target_link_libraries(bgl_benchmark CONAN_PKG::boost)
set_common_options(bgl_benchmark)

add_executable(bgl_csr_benchmark src/benchmark_bgl_csr.cpp)
set_target_properties(bgl_csr_benchmark PROPERTIES CXX_STANDARD 20)
target_link_libraries(bgl_csr_benchmark CONAN_PKG::boost)
set_common_options(bgl_csr_benchmark)                              
                                                                                              
# ################## MELON target ####################
add_executable(melon_benchmark src/benchmark_melon.cpp)
set_target_properties(melon_benchmark PROPERTIES CXX_STANDARD 20)
target_link_libraries(melon_benchmark CONAN_PKG::range-v3)
set_common_options(melon_benchmark)

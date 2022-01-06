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

# ################## LEMON target ####################

add_executable(lemon_benchmark src/lemon_benchmark.cpp)
set_target_properties(lemon_benchmark PROPERTIES CXX_STANDARD 17)
target_include_directories(lemon_benchmark PUBLIC ${LEMON_INCLUDE_DIRS})
target_link_libraries(lemon_benchmark ${LEMON_LIBRARIES})
target_compile_options_if_exists(lemon_benchmark PRIVATE ${WARNINGS}
                                 ${WARNINGS_LIST})
target_compile_options_if_exists(lemon_benchmark PRIVATE ${COMPILE_FOR_NATIVE}
                                 -march=native)
target_compile_options_if_exists(lemon_benchmark PRIVATE ${COMPILE_WITH_LTO}
                                 -flto)

# ################## BOOST target ####################
                            
add_executable(bgl_benchmark src/bgl_benchmark.cpp)
set_target_properties(bgl_benchmark PROPERTIES CXX_STANDARD 20)
target_link_libraries(bgl_benchmark CONAN_PKG::boost)
target_compile_options_if_exists(bgl_benchmark PRIVATE ${WARNINGS}
                                 ${WARNINGS_LIST})
target_compile_options_if_exists(bgl_benchmark PRIVATE ${COMPILE_FOR_NATIVE}
                                 -march=native)
target_compile_options_if_exists(bgl_benchmark PRIVATE ${COMPILE_WITH_LTO}
                                 -flto)
                                 
# ################## MELON target ####################

add_executable(melon_benchmark src/melon_benchmark.cpp)
set_target_properties(melon_benchmark PROPERTIES CXX_STANDARD 20)
target_include_directories(melon_benchmark PUBLIC include)
target_link_libraries(melon_benchmark CONAN_PKG::range-v3)
target_compile_options_if_exists(melon_benchmark PRIVATE ${WARNINGS}
                                 ${WARNINGS_LIST})
target_compile_options_if_exists(melon_benchmark PRIVATE ${COMPILE_FOR_NATIVE}
                                 -march=native)
target_compile_options_if_exists(melon_benchmark PRIVATE ${COMPILE_WITH_LTO}
                                 -flto)

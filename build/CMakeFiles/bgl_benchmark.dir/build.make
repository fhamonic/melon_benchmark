# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/plaiseek/Projects/melon_benchmark

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/plaiseek/Projects/melon_benchmark/build

# Include any dependencies generated for this target.
include CMakeFiles/bgl_benchmark.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/bgl_benchmark.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/bgl_benchmark.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/bgl_benchmark.dir/flags.make

CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o: CMakeFiles/bgl_benchmark.dir/flags.make
CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o: ../src/bgl_benchmark.cpp
CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o: CMakeFiles/bgl_benchmark.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/plaiseek/Projects/melon_benchmark/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o"
	/usr/bin/g++-10 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o -MF CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o.d -o CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o -c /home/plaiseek/Projects/melon_benchmark/src/bgl_benchmark.cpp

CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.i"
	/usr/bin/g++-10 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/plaiseek/Projects/melon_benchmark/src/bgl_benchmark.cpp > CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.i

CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.s"
	/usr/bin/g++-10 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/plaiseek/Projects/melon_benchmark/src/bgl_benchmark.cpp -o CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.s

# Object files for target bgl_benchmark
bgl_benchmark_OBJECTS = \
"CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o"

# External object files for target bgl_benchmark
bgl_benchmark_EXTERNAL_OBJECTS =

bin/bgl_benchmark: CMakeFiles/bgl_benchmark.dir/src/bgl_benchmark.cpp.o
bin/bgl_benchmark: CMakeFiles/bgl_benchmark.dir/build.make
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_contract.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_coroutine.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_fiber_numa.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_fiber.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_context.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_graph.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_iostreams.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_json.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_log_setup.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_log.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_locale.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_math_c99.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_math_c99f.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_math_c99l.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_math_tr1.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_math_tr1f.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_math_tr1l.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_nowide.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_program_options.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_random.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_regex.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_stacktrace_addr2line.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_stacktrace_backtrace.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_stacktrace_basic.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_stacktrace_noop.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_timer.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_type_erasure.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_thread.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_atomic.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_chrono.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_container.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_date_time.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_unit_test_framework.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_prg_exec_monitor.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_test_exec_monitor.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_exception.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_wave.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_filesystem.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_system.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_wserialization.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/boost/1.76.0/_/_/package/91068d6d6067a809e746602e9538e15eddb14457/lib/libboost_serialization.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/zlib/1.2.11/_/_/package/19729b9559f3ae196cad45cb2b97468ccb75dcd1/lib/libz.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/bzip2/1.0.8/_/_/package/91a8b22c2c5a149bc617cfc06cdd21bf23b12567/lib/libbz2.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/libiconv/1.16/_/_/package/19729b9559f3ae196cad45cb2b97468ccb75dcd1/lib/libiconv.a
bin/bgl_benchmark: /home/plaiseek/.conan/data/libiconv/1.16/_/_/package/19729b9559f3ae196cad45cb2b97468ccb75dcd1/lib/libcharset.a
bin/bgl_benchmark: CMakeFiles/bgl_benchmark.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/plaiseek/Projects/melon_benchmark/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/bgl_benchmark"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/bgl_benchmark.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/bgl_benchmark.dir/build: bin/bgl_benchmark
.PHONY : CMakeFiles/bgl_benchmark.dir/build

CMakeFiles/bgl_benchmark.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/bgl_benchmark.dir/cmake_clean.cmake
.PHONY : CMakeFiles/bgl_benchmark.dir/clean

CMakeFiles/bgl_benchmark.dir/depend:
	cd /home/plaiseek/Projects/melon_benchmark/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/plaiseek/Projects/melon_benchmark /home/plaiseek/Projects/melon_benchmark /home/plaiseek/Projects/melon_benchmark/build /home/plaiseek/Projects/melon_benchmark/build /home/plaiseek/Projects/melon_benchmark/build/CMakeFiles/bgl_benchmark.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/bgl_benchmark.dir/depend


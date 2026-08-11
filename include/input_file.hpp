#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

// Instance paths are resolved relative to the current working directory and no
// graph format here carries a length or a checksum, so an absent file is not
// an error any parser notices: the stream yields nothing, the header counts
// keep whatever the stack held, and the benchmark then sizes a graph from that
// garbage. What surfaces is a std::length_error, a segfault or a spin -- never
// the name of the file that was missing. Fail at the open, where the path is
// still known.
[[nodiscard]] inline std::ifstream open_input_file(
    const std::filesystem::path & file_name) {
    std::ifstream file(file_name);
    if(!file) {
        std::cerr << "error: cannot read instance " << file_name << '\n'
                  << "  Instance paths are relative to the working directory,"
                     " so run the benchmark\n"
                  << "  from the repository root, and fetch the datasets"
                     " first:\n"
                  << "    scripts/fetch_data.sh\n";
        std::exit(1);
    }
    return file;
}

#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "reference_solution.hpp"

// The DIMACS max-flow format declares its terminals with "n <id> s" and
// "n <id> t" lines, before the arcs. The benchmarks used to hardcode vertices
// 0 and 1, which happens to be right for BVZ-tsukuba and is not a property of
// the format.
[[nodiscard]] inline std::pair<std::size_t, std::size_t> max_flow_terminals(
    const std::filesystem::path & max_file) {
    std::ifstream f(max_file);
    std::size_t source = 0, sink = 1;
    std::string line;
    while(std::getline(f, line)) {
        std::istringstream iss(line);
        char ch;
        if(!(iss >> ch)) continue;
        if(ch == 'a') break;  // terminals are declared before the arc list
        if(ch != 'n') continue;
        std::size_t id;
        char which;
        if(iss >> id >> which) {
            if(which == 's')
                source = id - 1;
            else if(which == 't')
                sink = id - 1;
        }
    }
    return {source, sink};
}

// The max-flow instances' ".sol" optimum; see reference_solution_value.
[[nodiscard]] inline std::optional<long long> reference_flow_value(
    const std::filesystem::path & max_file) {
    return reference_solution_value(max_file);
}

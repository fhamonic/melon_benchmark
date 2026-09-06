#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

// Every instance family that ships a known optimum does it the same way: a
// sibling ".sol" file whose "s <value>" line holds the answer -- BVZ-tsukuba
// upstream, the generated RMF and NETGEN families from their own scripts.
// Benchmarks check against it, so an implementation cannot be merely fast.
// Returns nullopt when no reference file exists (e.g. the SNAP instances).
[[nodiscard]] inline std::optional<long long> reference_solution_value(
    const std::filesystem::path & instance_file) {
    std::filesystem::path sol_file = instance_file;
    sol_file.replace_extension(".sol");
    std::ifstream f(sol_file);
    if(!f) return std::nullopt;
    std::string line;
    while(std::getline(f, line)) {
        std::istringstream iss(line);
        char ch;
        if(iss >> ch && ch == 's') {
            long long value;
            if(iss >> value) return value;
        }
    }
    return std::nullopt;
}

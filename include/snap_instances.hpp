#pragma once

#include <filesystem>
#include <utility>
#include <vector>

#include "helper.hpp"

std::vector<std::tuple<std::filesystem::path, int, int>> instances = {
    {"data/snap/web-Stanford.txt", 281904, 2312497},
    {"data/snap/Amazon0302.txt", 262111, 1234877},
    {"data/snap/Amazon0505.txt", 410236, 3356824},
    {"data/snap/WikiTalk.txt", 2394385, 5021410}};

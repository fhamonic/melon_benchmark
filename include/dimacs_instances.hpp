#pragma once

#include <filesystem>
#include <utility>
#include <vector>

#include "helper.hpp"

std::vector<std::tuple<std::filesystem::path, int, int>> instances = {
    {"data/9th_DIMACS_USA_roads/rome99.gr", 3353, 8870},
    // {"data/9th_DIMACS_USA_roads/distance/USA-road-d.NY.gr", 264346, 733846},
    {"data/9th_DIMACS_USA_roads/time/USA-road-t.NY.gr", 264346, 733846},
    // {"data/9th_DIMACS_USA_roads/distance/USA-road-d.BAY.gr", 321270, 800172},
    {"data/9th_DIMACS_USA_roads/time/USA-road-t.BAY.gr", 321270, 800172},
    // {"data/9th_DIMACS_USA_roads/distance/USA-road-d.COL.gr", 435666, 1057066},
    {"data/9th_DIMACS_USA_roads/time/USA-road-t.COL.gr", 435666, 1057066},
    // {"data/9th_DIMACS_USA_roads/distance/USA-road-d.FLA.gr", 1070376, 2712798},
    {"data/9th_DIMACS_USA_roads/time/USA-road-t.FLA.gr", 1070376, 2712798},
    // {"data/9th_DIMACS_USA_roads/distance/USA-road-d.NW.gr", 1207945, 2840208},
    {"data/9th_DIMACS_USA_roads/time/USA-road-t.NW.gr", 1207945, 2840208},
    // {"data/9th_DIMACS_USA_roads/distance/USA-road-d.NE.gr", 1524453, 3897636},
    {"data/9th_DIMACS_USA_roads/time/USA-road-t.NE.gr", 1524453, 3897636}};

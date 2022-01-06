#include <iostream>

#include "melon.hpp"

namespace fhamonic::melon;

int main() {
    std::ifstream gr_file("data/rome99.gr");

    StaticDigraphBuilder<double> builder;

    std::string line;
    while (getline(dimacs, line))
    {
        std::istringstream iss(line);
        char ch;
        if (iss >> ch)
        {
            size_t from, to;
            std::string format;

            switch(ch) {
                case 'c': break;
                case 'p': 
                    if (vertices||edges) return false;
                    if (iss >> format >> vertices >> edges) {
                        if ("edge" != format) return false;
                    }
                    break;
                case 'e': 
                    if (edges-- && (iss >> from >> to) && (add_edge(from-1, to-1, g).second))
                        break;
                default: 
                    return false;
            }
        }
    }

    return !(edges || !dimacs.eof());


    return 0;
}
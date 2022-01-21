#ifndef WARM_UP_HPP
#define WARM_UP_HPP

#include "chrono.hpp"

int warm_up() {
    Chrono chrono;
    int a,b,c,d;
    a = b = c = d = 1;
    while(chrono.timeMs() < 10) {
        a = a*b;
        ++b;
        c = d-a;
        --d;
    }
    return d;
}

#endif  // WARM_UP_HPP
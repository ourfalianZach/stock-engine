#pragma once // replacement for #ifndef and #define include guards

#include <cstdint>
#include "side.hpp"

struct Order{
    uint64_t id; // unique order identifier
    Side side;
    int64_t price; // price in cents
    int64_t qty;
};
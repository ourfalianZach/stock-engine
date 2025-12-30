#pragma once // replacement for #ifndef and #define include guards

#include <cstdint>

struct Trade{
    int64_t price; // price in cents
    int64_t qty;
    uint64_t taker_order_id; // the incoming order
    uint64_t maker_order_id; // the order that was already sitting in the book
};
#pragma once // replacement for #ifndef and #define include guards

// enum just means the Side object is one of the two values in the list 'Buy' or 'Sell'
// i.e., Side s = Side::Buy; or Side s2 = Side::Sell;

enum class Side { 
    Buy,
    Sell
};


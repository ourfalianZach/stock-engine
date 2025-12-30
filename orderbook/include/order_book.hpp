#pragma once // replacement for #ifndef and #define include guards

#include <cstdint>
#include <deque>
#include "order.hpp"
#include <map>
#include <functional> // std::greater
#include <optional>
#include "trade.hpp"
#include <vector>


struct PriceLevel {
    int64_t price; // cents
    int64_t total_qty; // total quantity at this price level
};

struct BookSnapshot{
    std::vector<PriceLevel> bids; // best to worst
    std::vector<PriceLevel> asks; // best to worst
};

class OrderBook {
public:
    // add a limit order to the book without matching (for now)
    void add(const Order& order);

    // matches first, then rests remaining qty. returns resulting trades
    std::vector<Trade> add_and_match(Order order); // order is the new order to add and match

    // best prices
    std::optional<int64_t> best_bid() const; // maybe it contains a price maybe it doesn't
    std::optional<int64_t> best_ask() const;

    // convenience: is the book empty on a side?
    bool has_bids() const {return !bids_.empty();}
    bool has_asks() const {return !asks_.empty();}

    BookSnapshot snapshot(std::size_t depth = 10) const;

    bool cancel(uint64_t order_id); // returns true if found and cancelled
    
    std::vector<Trade> recent_trades(std::size_t limit = 50) const;
    void clear_trades();

private:
    // asks: lowest price first
    std::map<int64_t, std::deque<Order>> asks_; // price -> orders at that price

    // bids: highest price first
    std::map<int64_t, std::deque<Order>, std::greater<int64_t>> bids_; // price -> orders at that price
    // instead of smallest to largest, we use largest to smallest (i.e., std::greater)

    std::deque<Trade> trade_log_;
    std::size_t trade_log_cap = 1000;
};
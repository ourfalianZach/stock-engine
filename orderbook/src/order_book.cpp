#include <order_book.hpp>

void OrderBook::add(const Order& order) {
    if (order.side == Side::Buy) {
        bids_[order.price].push_back(order); // FIFO at that price
    } else {
        asks_[order.price].push_back(order);
    }
}

std::vector<Trade> OrderBook::add_and_match(Order order) {
  std::vector<Trade> trades;

  // BUY order matches against ASKS (lowest first)
  if (order.side == Side::Buy) {
    while (order.qty > 0 && !asks_.empty()) { // incoming order has remaining qty and there are asks to match against
      auto best_ask_it = asks_.begin(); // lowest ask
      int64_t best_ask_price = best_ask_it->first; // first is the key (price) second is the value (deque of orders at that price)

      // If we can't cross the spread, stop matching.
      if (best_ask_price > order.price) break; // we won't buy at a price higher than our limit price (first price is lowest so all others are higher)

      auto& queue = best_ask_it->second; // FIFO at that price (queue of orders at best ask price)

      while (order.qty > 0 && !queue.empty()) {
        Order& maker = queue.front(); // the resting sell order (liquidity provider)
        int64_t trade_qty = (order.qty < maker.qty) ? order.qty : maker.qty; // if incoming order qty is less than resting order qty, 
                                                                            // trade qty is incoming order qty, else it's resting order qty

        trades.push_back(Trade{
          .price = best_ask_price,
          .qty = trade_qty,
          .taker_order_id = order.id,
          .maker_order_id = maker.id
        });

        order.qty -= trade_qty;
        maker.qty -= trade_qty;

        if (maker.qty == 0) { // if resting order is fully filled, remove it from the queue
          queue.pop_front();
        }
      } // end inner while (keep repeating until incoming order qty is 0 or no more orders at this price)

      // If no orders remain at this price level, remove the level from the map.
      if (queue.empty()) {
        asks_.erase(best_ask_it);
      }
    }

    // if there's remaining qty on the incoming order, add it to the bids as the newest order at that price
    if (order.qty > 0) {
      bids_[order.price].push_back(order);
    }
  }
  // SELL order matches against BIDS (highest first)
  else {
    while (order.qty > 0 && !bids_.empty()) {
      auto best_bid_it = bids_.begin(); // highest bid (because std::greater)
      int64_t best_bid_price = best_bid_it->first;

      if (best_bid_price < order.price) break;

      auto& queue = best_bid_it->second;

      while (order.qty > 0 && !queue.empty()) {
        Order& maker = queue.front(); // resting buy order
        int64_t trade_qty = (order.qty < maker.qty) ? order.qty : maker.qty;

        trades.push_back(Trade{
          .price = best_bid_price,
          .qty = trade_qty,
          .taker_order_id = order.id,
          .maker_order_id = maker.id
        });

        order.qty -= trade_qty;
        maker.qty -= trade_qty;

        if (maker.qty == 0) {
          queue.pop_front();
        }
      }

      if (queue.empty()) {
        bids_.erase(best_bid_it);
      }
    }

    if (order.qty > 0) {
      asks_[order.price].push_back(order);
    }
  }

  return trades;
}



std::optional<int64_t> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt; // no bids
    }
    return bids_.begin()->first; // highest price due to std::greater
}

std::optional<int64_t> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt; // no asks
    }
    return asks_.begin()->first; // lowest price
}

BookSnapshot OrderBook::snapshot(std::size_t depth) const {
    BookSnapshot snap;

    for(auto it = bids_.begin(); it != bids_.end() && snap.bids.size() < depth; ++it) { // loop through bids until we reach desired depth
        int64_t total_qty = 0;
        for (const auto& order : it->second) { // sum up quantities at this price level from the deque
            total_qty += order.qty;
        }
        snap.bids.push_back(PriceLevel{.price = it->first, .total_qty = total_qty});
    }

    for(auto it = asks_.begin(); it != asks_.end() && snap.asks.size() < depth; ++it) {
        int64_t total_qty = 0;
        for (const auto& order : it->second) {
            total_qty += order.qty;
        }
        snap.asks.push_back(PriceLevel{.price = it->first, .total_qty = total_qty});
    }

    return snap;

}

bool OrderBook::cancel(uint64_t order_id){
    // Search bids
    for(auto it = bids_.begin(); it != bids_.end(); ++it)
    {
      auto& q = it->second; // deque at this price
      for(auto qi = q.begin(); qi != q.end(); ++qi)
      {
        if(qi->id == order_id)
        {
          q.erase(qi); // remove desired order
          if(q.empty())
            bids_.erase(it); // remove the whole price level if empty
          return true;
        }
      }
    }

    // Search asks
    for (auto it = asks_.begin(); it != asks_.end(); ++it) {
      auto& q = it->second;
      for (auto qi = q.begin(); qi != q.end(); ++qi) {
        if (qi->id == order_id) {
          q.erase(qi);
          if (q.empty()) 
            asks_.erase(it);
          return true;
        }
      }
    }
    return false; // if not found
}
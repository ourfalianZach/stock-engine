#include <iostream>
#include "order_book.hpp"

static void print_trades(const std::vector<Trade>& trades) {
  for (const auto& t : trades) {
    std::cout << "TRADE price=" << t.price
              << " qty=" << t.qty
              << " taker=" << t.taker_order_id
              << " maker=" << t.maker_order_id
              << "\n";
  }
}

static void print_snapshot(const BookSnapshot& s) {
  std::cout << "BIDS (best->worse):\n";
  for (const auto& lvl : s.bids) {
    std::cout << "  price=" << lvl.price << " total_qty=" << lvl.total_qty << "\n";
  }

  std::cout << "ASKS (best->worse):\n";
  for (const auto& lvl : s.asks) {
    std::cout << "  price=" << lvl.price << " total_qty=" << lvl.total_qty << "\n";
  }
}

int main() {
  OrderBook book;

  // Add two bids at same price level
  book.add_and_match(Order{.id=10, .side=Side::Buy, .price=10000, .qty=5});
  book.add_and_match(Order{.id=11, .side=Side::Buy, .price=10000, .qty=7});

  std::cout << "Before cancel:\n";
  print_snapshot(book.snapshot(5));

  bool ok = book.cancel(10);
  std::cout << "Cancel(10) => " << (ok ? "true" : "false") << "\n";

  std::cout << "After cancel:\n";
  print_snapshot(book.snapshot(5));
}
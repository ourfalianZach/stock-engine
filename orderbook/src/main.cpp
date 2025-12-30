#include <iostream>
#include "order_book.hpp"


#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

//covert BookSnapshot to JSON
static json snapshot_to_json(const BookSnapshot& snap) {
  json j;
  j["bids"] = json::array();
  j["asks"] = json::array();

  for (const auto& lvl : snap.bids) {
    j["bids"].push_back({
      {"price", lvl.price},        // cents
      {"qty",   lvl.total_qty}
    });
  }

  for (const auto& lvl : snap.asks) {
    j["asks"].push_back({
      {"price", lvl.price},        // cents
      {"qty",   lvl.total_qty}
    });
  }

  return j;
}
// parse depth with a default, fallback if invalid 
static std::size_t depth_or_default(const httplib::Request& req, std::size_t def) {
  if (!req.has_param("depth")) return def;
  try {
    return static_cast<std::size_t>(std::stoul(req.get_param_value("depth")));
  } catch (...) {
    return def;
  }
}

static json trades_to_json(const std::vector<Trade>& trades) {
  json arr = json::array();
  for (const auto& t : trades) {
    arr.push_back({
      {"price", t.price},
      {"qty", t.qty},
      {"taker_order_id", t.taker_order_id},
      {"maker_order_id", t.maker_order_id}
    });
  }
  return arr;
}

static std::size_t limit_or_default(const httplib::Request& req, std::size_t def) {
  if (!req.has_param("limit")) return def;
  try {
    return static_cast<std::size_t>(std::stoul(req.get_param_value("limit")));
  } catch (...) {
    return def;
  }
}

int main() {
  httplib::Server app;
  OrderBook book;

  std::atomic<uint64_t> next_order_id{100};

  book.add_and_match(Order{.id=1,  .side=Side::Sell, .price=10100, .qty=10});
  book.add_and_match(Order{.id=2,  .side=Side::Sell, .price=10200, .qty=10});
  book.add_and_match(Order{.id=10, .side=Side::Buy,  .price=10000, .qty=5});
  book.add_and_match(Order{.id=11, .side=Side::Buy,  .price=10000, .qty=7});



  // Route: GET /health
  app.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
    (void)req; // unused
    json j;
    j["status"] = "ok";
    res.set_content(j.dump(2), "application/json");
  });

  // Route: GET /book
  app.Get("/book", [&](const httplib::Request& req, httplib::Response& res) {
    std::size_t depth = depth_or_default(req, 10); 
    auto snap = book.snapshot(depth);
    auto j = snapshot_to_json(snap);
    res.set_content(j.dump(2), "application/json");
  });

  // Route: POST /orders
  app.Post("/orders", [&](const httplib::Request& req, httplib:: Response& res){
    json body;

    //step 1: if the input is not in the correct format end
    try{
      body = json::parse(req.body);
    } catch(...) {
      res.status = 400; // 400 means bad request default is 200 which means success
      res.set_content(R"({"error":"invalid JSON body})", "application/json");
      return;
    }

    // step 2: validate required fields
    if(!body.contains("side") || !body.contains("price") || !body.contains("qty")) {
      res.status = 400;
      res.set_content(R"({"error" : "missing required fields: side, price, qty"})", "application/json");
      return;
    }

    // step 3: validate types
    if (!body["side"].is_string() || !body["price"].is_number_integer() || !body["qty"].is_number_integer()) {
      res.status = 400;
      res.set_content(R"({"error":"invalid field types (side:string, price:int, qty:int"})", "application/json");
      return;
    }

    // step 4: parse side
    std::string side_s = body["side"].get<std::string>();
    Side side;
    if(side_s == "buy" || side_s == "Buy" || side_s == "BUY")
      side = Side::Buy;
    else if (side_s == "sell" || side_s == "Sell" || side_s == "SELL")
      side = Side::Sell;
    else{
      res.status = 400;
      res.set_content(R"({"error":"side must be 'buy' or 'sell'"})", "application/json");
      return;
    }

    // step 5: parse numeric fields (price & qty)

    int64_t price = body["price"].get<int64_t>();
    int64_t qty = body["qty"].get<int64_t>();
    if(price <= 0 || qty <= 0){
      res.status = 400;
      res.set_content(R"({"error":"price and qty must be positive"})", "application/json");
      return;
    }

    // step 6: create order & match
    uint64_t id = next_order_id.fetch_add(1);
    Order o{.id = id, .side = side, .price = price, .qty = qty};
    auto trades = book.add_and_match(o);

    // step 7: build response: order_id + trades + updated book snapshot
    std:size_t depth = depth_or_default(req, 10);
    auto snap = book.snapshot(depth);
    json out;
    out["order_id"] = id;
    out["trades"] = trades_to_json(trades);
    out["book"] = snapshot_to_json(snap);


    res.set_content(out.dump(2), "application/json");

  });


  app.Delete(R"(/orders/(\d+))", [&](const httplib::Request& req, httplib::Response& res){
    uint64_t id = 0;
    try{
      id = static_cast<uint64_t>(std::stoull(req.matches[1])); // req.matches[1] is the parameter passed in "/orders/id" (req.matches[1] = id)
    } catch(...) {
      res.status = 400;
      res.set_content(R"({"error":"invalid order id"})", "application/json");
      return;
    }

    bool ok = book.cancel(id);

    if(!ok){
      res.status = 404;
      res.set_content(R"({"error":"order not found"})", "application/json");
      return;
    }

    json out;
    out["cancelled"] = true;
    out["order_id"] = id;

    std::size_t depth = depth_or_default(req, 10);
    out["book"] = snapshot_to_json(book.snapshot(depth));

    res.set_content(out.dump(2), "application/json");
  });

  app.Get("/trades", [&](const httplib::Request& req, httplib::Response& res){ // /trades?limit=1"
    std::size_t limit = limit_or_default(req,50);
    auto trades = book.recent_trades(limit);
    json out;
    out["trades"] = trades_to_json(trades);
    res.set_content(out.dump(2), "application/json");
  });

  std::cout << "Server listening on http://localhost:8080\n";
  app.listen("0.0.0.0", 8080); // 0.0.0.0 = listen on all interfaces
  return 0;
}
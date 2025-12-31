"use client"; // without this we can't use useEffect or useState

import { useEffect, useState } from "react";

type Level = { price: number; qty: number }; // a single price level in the book
type Book = { bids: Level[]; asks: Level[] }; // the book itself with array for bids and asks

type Trade = {
  price: number;
  qty: number;
  taker_order_id: number;
  maker_order_id: number;
};

type PostOrderResponse = { // POST /orders response , assigned order_id of new order, trades that happened, updated book snapshot
  order_id: number;
  trades: Trade[];
  book: Book;
};

export default function Home() {
  const [book, setBook] = useState<Book>({ bids: [], asks: [] }); // book holds current order book, setBook updates the book and triggers re-render
  const [error, setError] = useState<string | null>(null); // error state for GET /book, either holds error message or null

  // Order form state
  const [side, setSide] = useState<"buy" | "sell">("buy"); // stores side, only allows buy or sell, default is buy
  const [price, setPrice] = useState<number>(10200); // stores price, must be a number, default is 10200
  const [qty, setQty] = useState<number>(1); // stores qty, must be a number, default is 1

  // Results state 
  const [lastOrderId, setLastOrderId] = useState<number | null>(null);
  const [lastTrades, setLastTrades] = useState<Trade[]>([]);
  const [submitError, setSubmitError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  // Poll the book
  useEffect(() => {
    let alive = true;

    async function fetchBook() {
      try {
        const res = await fetch("http://localhost:8080/book?depth=10"); // gets the book
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const data = (await res.json()) as Book; // parse JSON into JS object
        if (alive) { // update data only if alive
          setBook(data);
          setError(null);
        }
      } catch (err) {
        if (!alive) return;
        setError(err instanceof Error ? err.message : "Failed to fetch");
      }
    }

    fetchBook();
    const id = setInterval(fetchBook, 500); // every 500 ms it runs fetchBook
    return () => {
      alive = false;
      clearInterval(id); // stop the timer 
    };
  }, []);

  async function submitOrder(e: React.FormEvent) { // the function runs when a form is submitted (e is the browser's submit event)
    e.preventDefault(); // prevents the page from reloading (default behavior)
    setSubmitting(true);
    setSubmitError(null);

    try { // await pauses the function until response arrives
      const res = await fetch("http://localhost:8080/orders?depth=10", { // sends the POST request
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ side, price, qty }),
      });

      const text = await res.text(); // read response as text
      if (!res.ok) {
        // backend returns JSON like {"error":"..."}
        try {
          const j = JSON.parse(text) as { error?: string };
          throw new Error(j.error ?? `HTTP ${res.status}`); // throw error extract the backend error message
        } catch {
          throw new Error(`HTTP ${res.status}`);
        }
      }

      const data = JSON.parse(text) as PostOrderResponse; // parse successful backend response
      setLastOrderId(data.order_id);
      setLastTrades(data.trades);
      setBook(data.book);

    } catch (err) {
      setSubmitError(err instanceof Error ? err.message : "Failed to submit");
    } finally {
      setSubmitting(false); // exit submitting state at the end
    }
  }

  return (
    <main className="p-8 font-sans">  {/* this is the page container (p = padding) */}
      <h1 className="text-2xl font-bold mb-4">Stock Engine</h1> {/* Page title  (mb = margin bottom)*/}

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8"> {/*  grid layout for page. large screen 3 cols, small screens 1 col (form, bids, asks) */}
        {/* Order form */}
        <section className="border rounded-lg p-4"> {/* just styling the order form box */}
          <h2 className="text-lg font-semibold mb-3">Place Order</h2> {/* box title */}

          <form onSubmit={submitOrder} className="space-y-3"> {/* when user clicks submit 1. browser fires a submit event 2. react intercepts it 3. calls submitOrder(e) */}
            <div>
              <label className="block text-sm mb-1">Side</label> {/* side label and drop down */}
              <select
                className="border rounded px-2 py-1 w-full"  
                value={side}   // react controls this value
                onChange={(e) => setSide(e.target.value as "buy" | "sell")} // changing dropdown triggers onChange, calls setSide, updates state, causes re-render, dropdown shows new value
              >
                <option value="buy">buy</option>
                <option value="sell">sell</option>
              </select>
            </div>

            <div>
              <label className="block text-sm mb-1">Price (cents)</label> {/* price label and text box works the same as side */}
              <input
                className="border rounded px-2 py-1 w-full"
                type="number"
                value={price}
                onChange={(e) => setPrice(Number(e.target.value))}
                min={1}
              />
            </div>

            <div>
              <label className="block text-sm mb-1">Quantity</label> {/* qty label and text box works the same as side */}
              <input
                className="border rounded px-2 py-1 w-full"
                type="number"
                value={qty}
                onChange={(e) => setQty(Number(e.target.value))}
                min={1}
              />
            </div>

            <button // submit button
              className="bg-black text-white rounded px-3 py-2 w-full disabled:opacity-50"
              disabled={submitting}
              type="submit"
            >
              {submitting ? "Submitting..." : "Submit"} {/* if submitting: button disabled & label = "Submitting..." if false button enabled & label = "Submit" */}
            </button>

            {submitError && <p className="text-red-600 text-sm">{submitError}</p>} {/* if submitError is null does nothing if it's a string renders string as a <p> */}
          </form>

          {/* last result panel */}
          <div className="mt-4">
            <h3 className="font-semibold">Last Result</h3>
            <p className="text-sm">Order ID: {lastOrderId ?? "(none)"}</p> {/* if lastOrderId == null show "(none)" else show the number*/}

            <div className="mt-2">
              <p className="text-sm font-semibold">Trades</p> 
              {lastTrades.length === 0 ? (
                <p className="text-sm opacity-70">(none)</p> // if there are no trades made from incoming order show null 
              ) : ( // else
                <ul className="text-sm space-y-1 mt-1">
                  {lastTrades.map((t, i) => (       // loop over list of trades (each t is a trade object from backend)
                    <li key={i} className="flex justify-between">
                      <span>px {t.price}</span>   {/* print orders price, qty, & maker_order_id */}
                      <span>qty {t.qty}</span>
                      <span className="opacity-70">
                        maker {t.maker_order_id} 
                      </span>
                    </li>
                  ))}
                </ul>
              )}
            </div>
          </div>
        </section>

        {/* Order book */}
        <section className="border rounded-lg p-4 lg:col-span-2">
          <h2 className="text-lg font-semibold mb-2">Order Book</h2>

          {error && <p className="text-red-600 mb-2">Error: {error}</p>} {/* only show error if it exists */}

          <div className="grid grid-cols-2 gap-8">
            <div>
              <h3 className="font-semibold text-green-600 mb-2">Bids</h3>
              <div className="space-y-1">
                {book.bids.map((b) => ( // render through bids printing each price and qty (each bid 'b')
                  <div key={`b-${b.price}`} className="flex justify-between">
                    <span>{b.price}</span>
                    <span>{b.qty}</span>
                  </div>
                ))}
              </div>
            </div>

            <div>
              <h3 className="font-semibold text-red-600 mb-2">Asks</h3>
              <div className="space-y-1">
                {book.asks.map((a) => ( // render through asks printing each price and qty
                  <div key={`a-${a.price}`} className="flex justify-between">
                    <span>{a.price}</span>
                    <span>{a.qty}</span>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </section>
      </div>
    </main>
  );
}
"use client";

import { useEffect, useState } from "react";

type Level = { price: number; qty: number };
type Book = { bids: Level[]; asks: Level[] };

type Trade = {
  price: number;
  qty: number;
  taker_order_id: number;
  maker_order_id: number;
};

type PostOrderResponse = {
  order_id: number;
  trades: Trade[];
  book: Book;
};

export default function Home() {
  const [book, setBook] = useState<Book>({ bids: [], asks: [] });
  const [error, setError] = useState<string | null>(null);

  // Order form state
  const [side, setSide] = useState<"buy" | "sell">("buy");
  const [price, setPrice] = useState<number>(10200);
  const [qty, setQty] = useState<number>(1);

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
        const res = await fetch("http://localhost:8080/book?depth=10");
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const data = (await res.json()) as Book;
        if (alive) {
          setBook(data);
          setError(null);
        }
      } catch (err) {
        if (!alive) return;
        setError(err instanceof Error ? err.message : "Failed to fetch");
      }
    }

    fetchBook();
    const id = setInterval(fetchBook, 500);
    return () => {
      alive = false;
      clearInterval(id);
    };
  }, []);

  async function submitOrder(e: React.FormEvent) {
    e.preventDefault();
    setSubmitting(true);
    setSubmitError(null);

    try {
      const res = await fetch("http://localhost:8080/orders?depth=10", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ side, price, qty }),
      });

      const text = await res.text();
      if (!res.ok) {
        // backend returns JSON like {"error":"..."}
        try {
          const j = JSON.parse(text) as { error?: string };
          throw new Error(j.error ?? `HTTP ${res.status}`);
        } catch {
          throw new Error(`HTTP ${res.status}`);
        }
      }

      const data = JSON.parse(text) as PostOrderResponse;
      setLastOrderId(data.order_id);
      setLastTrades(data.trades);

      // Optional: update book immediately (polling will also refresh it)
      setBook(data.book);
    } catch (err) {
      setSubmitError(err instanceof Error ? err.message : "Failed to submit");
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <main className="p-8 font-sans">
      <h1 className="text-2xl font-bold mb-4">Stock Engine</h1>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        {/* Order form */}
        <section className="border rounded-lg p-4">
          <h2 className="text-lg font-semibold mb-3">Place Order</h2>

          <form onSubmit={submitOrder} className="space-y-3">
            <div>
              <label className="block text-sm mb-1">Side</label>
              <select
                className="border rounded px-2 py-1 w-full"
                value={side}
                onChange={(e) => setSide(e.target.value as "buy" | "sell")}
              >
                <option value="buy">buy</option>
                <option value="sell">sell</option>
              </select>
            </div>

            <div>
              <label className="block text-sm mb-1">Price (cents)</label>
              <input
                className="border rounded px-2 py-1 w-full"
                type="number"
                value={price}
                onChange={(e) => setPrice(Number(e.target.value))}
                min={1}
              />
            </div>

            <div>
              <label className="block text-sm mb-1">Quantity</label>
              <input
                className="border rounded px-2 py-1 w-full"
                type="number"
                value={qty}
                onChange={(e) => setQty(Number(e.target.value))}
                min={1}
              />
            </div>

            <button
              className="bg-black text-white rounded px-3 py-2 w-full disabled:opacity-50"
              disabled={submitting}
              type="submit"
            >
              {submitting ? "Submitting..." : "Submit"}
            </button>

            {submitError && <p className="text-red-600 text-sm">{submitError}</p>}
          </form>

          <div className="mt-4">
            <h3 className="font-semibold">Last Result</h3>
            <p className="text-sm">Order ID: {lastOrderId ?? "(none)"}</p>

            <div className="mt-2">
              <p className="text-sm font-semibold">Trades</p>
              {lastTrades.length === 0 ? (
                <p className="text-sm opacity-70">(none)</p>
              ) : (
                <ul className="text-sm space-y-1 mt-1">
                  {lastTrades.map((t, i) => (
                    <li key={i} className="flex justify-between">
                      <span>px {t.price}</span>
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

          {error && <p className="text-red-600 mb-2">Error: {error}</p>}

          <div className="grid grid-cols-2 gap-8">
            <div>
              <h3 className="font-semibold text-green-600 mb-2">Bids</h3>
              <div className="space-y-1">
                {book.bids.map((b) => (
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
                {book.asks.map((a) => (
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
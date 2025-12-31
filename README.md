High-Performance Trading Engine & Order Book (C++ & Next.js)

    A high-performance limit order book and matching engine implemented in C++, with a React / Next.js frontend for live visualization of the order book, trades, and order execution.
    This project simulates core mechanics of a real electronic exchange, including price-time priority matching, partial fills, and REST-based order submission.

Features:

C++ Matching Engine
	•	Limit order book with price-time priority
	•	Partial and multi-level fills
	•	Order cancellation
	•	In-memory trade tracking
REST API (C++)
	•	POST /orders — submit buy/sell orders
	•	GET /book — aggregated order book snapshot
	•	GET /trades — recent trade history
	•	DELETE /orders/{id} — cancel open orders
Frontend (Next.js + TypeScript)
	•	Live order book view (bids & asks)
	•	Order entry form
	•	Immediate execution report (trades per order)
	•	Auto-updating via polling


	<img width="1902" height="615" alt="image" src="https://github.com/user-attachments/assets/6d3b96af-bca7-4f37-8bd5-45e973e880b8" />


# NANOMATCH — Ultra-Low Latency Order Matching Engine

A fully functional **Limit Order Book (LOB)** and matching engine written in C++17.
Processes buy/sell orders on strict price-time priority with sub-microsecond matching latency.

Built as part of FEC IIT Guwahati DIY '26 — Quant · Systems track.

---

## What This Is

Every stock exchange — NSE, NASDAQ, BSE — runs an order matching engine at its core.
When you place a trade, your order enters a Limit Order Book and gets matched against
the best available opposing order. NANOMATCH implements this from scratch in C++,
targeting the same architectural principles used by HFT firms.

---

## Benchmark Results

Hardware: 16-core x86_64, L1=48KB, L2=1280KB, L3=24MB, WSL2 Ubuntu 24.04

| Benchmark | Time | What it measures |
|-----------|------|-----------------|
| BM_OrderRest | 86 ns | Order insert, no match |
| BM_OrderMatch | 114 ns | Complete trade execution |
| BM_RingBuffer | 7 ns | Lock-free event logging |
| p50 (real workload) | 328 ns | Median over 100K orders |
| p99 (real workload) | 51,742 ns | Heavy multi-fill orders |
| Throughput | ~137K orders/sec | End-to-end with CSV parsing |

---

## Architecture
┌─────────────────────────────────────────────────┐
│              DATA INGESTION                     │
│   CSV file → parser → MemoryPool<Order>        │
└─────────────────────┬───────────────────────────┘
│ Order*
┌─────────────────────▼───────────────────────────┐
│              ORDER BOOK CORE                    │
│  Bid Side                    Ask Side           │
│  vector<PriceLevel>          vector<PriceLevel> │
│  sorted descending           sorted ascending   │
│         ↓   MATCHING ENGINE   ↓                 │
│       price-time priority matching              │
└─────────────────────┬───────────────────────────┘
│ TradeEvent
┌─────────────────────▼───────────────────────────┐
│         LOCK-FREE SPSC RING BUFFER              │
│   atomic head/tail → logger thread             │
└─────────────────────────────────────────────────┘
---

## Why These Decisions

**alignas(64) on Order struct**
Every Order is exactly 64 bytes — one CPU cache line. The CPU fetches it in a
single memory operation. Without alignment an Order could span two cache lines,
doubling memory latency per access.

**Memory Pool (MemoryPool<Order, 200000>)**
malloc() takes ~200ns per call. The pool pre-allocates 200K Order slots at
startup and hands them out in ~5ns via a free-list. Zero heap allocations
on the hot path.

**Sorted vector instead of std::map**
std::map is a red-black tree — every node is a separate heap allocation.
Accessing 10 price levels means 10 pointer chases and 10 potential cache misses.
A sorted vector of 10 levels fits in one cache line. Linear scan beats tree
lookup for the small number of active price levels in a real order book.

**Lock-free SPSC Ring Buffer**
A mutex causes a futex syscall (~1-10 us) when contended — longer than the
entire match cycle. The ring buffer uses atomic head/tail with release/acquire
ordering. Zero kernel involvement. Zero blocking. head_ and tail_ each have
their own alignas(64) cache line to prevent false sharing between cores.

**Callgrind Profiling Result**
44% of CPU time in benchmarks was malloc/free — confirming the memory pool
is the single highest-impact optimization for production workloads.

---

## File Structure
---

## Why These Decisions

**alignas(64) on Order struct**
Every Order is exactly 64 bytes — one CPU cache line. The CPU fetches it in a
single memory operation. Without alignment an Order could span two cache lines,
doubling memory latency per access.

**Memory Pool (MemoryPool<Order, 200000>)**
malloc() takes ~200ns per call. The pool pre-allocates 200K Order slots at
startup and hands them out in ~5ns via a free-list. Zero heap allocations
on the hot path.

**Sorted vector instead of std::map**
std::map is a red-black tree — every node is a separate heap allocation.
Accessing 10 price levels means 10 pointer chases and 10 potential cache misses.
A sorted vector of 10 levels fits in one cache line. Linear scan beats tree
lookup for the small number of active price levels in a real order book.

**Lock-free SPSC Ring Buffer**
A mutex causes a futex syscall (~1-10 us) when contended — longer than the
entire match cycle. The ring buffer uses atomic head/tail with release/acquire
ordering. Zero kernel involvement. Zero blocking. head_ and tail_ each have
their own alignas(64) cache line to prevent false sharing between cores.

**Callgrind Profiling Result**
44% of CPU time in benchmarks was malloc/free — confirming the memory pool
is the single highest-impact optimization for production workloads.

---

## File Structure
---

## How to Build and Run

Requirements: g++ 12+, cmake 3.20+, Linux or WSL2

```bash
git clone https://github.com/babbyy-shark/nanomatch.git
cd nanomatch
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
python3 data/synthetic/gen_orders.py --orders 100000 --output orders.csv
./build/nanomatch data/synthetic/orders.csv
./build/nanomatch_bench
```

---

## Key Engineering Challenges

**1. Cache alignment**
Callgrind confirmed unaligned structs caused measurable overhead.
alignas(64) + static_assert(sizeof(Order)==64) enforces this at compile time.

**2. Quantity conservation bug**
assert() is silently disabled in Release builds via -DNDEBUG.
Replaced with explicit if-checks that work in all build modes.

**3. Const correctness**
BestBid()/BestAsk() on const OrderBook required adding const overloads
of Bids() and Asks(). The compiler caught this — prevents accidental
mutation through const references.

**4. False sharing prevention**
head_ and tail_ in the ring buffer each get their own alignas(64) cache line.
Without this, the producer updating tail_ invalidates the consumer's cached
copy of head_ on every operation.

---

## Concepts Demonstrated

- CPU cache hierarchy and cache line alignment
- Custom arena memory allocator (placement new, free-list)
- Lock-free concurrency with std::atomic and memory ordering
- Price-time priority matching algorithm
- SPSC ring buffer for producer-consumer decoupling
- Callgrind profiling and bottleneck identification
- Google Benchmark for statistical latency measurement

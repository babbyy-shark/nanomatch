# NANOMATCH — Ultra-Low Latency Order Matching Engine

> A fully functional Limit Order Book (LOB) and matching engine written in C++17.
> Built from scratch as part of **FEC IIT Guwahati DIY '26 — Quant · Systems track.**

---

## Table of Contents

1. [What This Is](#what-this-is)
2. [Benchmark Results](#benchmark-results)
3. [Architecture](#architecture)
4. [File Structure](#file-structure)
5. [Why These Decisions](#why-these-decisions)
6. [Key Engineering Challenges](#key-engineering-challenges)
7. [Concepts Demonstrated](#concepts-demonstrated)
8. [How to Build and Run](#how-to-build-and-run)

---

## What This Is

Every stock exchange — NSE, NASDAQ, BSE — runs an order matching engine at its core.
When you place a trade, your order enters a Limit Order Book and gets matched against
the best available opposing order. NANOMATCH implements this from scratch in C++,
targeting the same architectural principles used by HFT firms.

The goal: process buy/sell orders on strict **price-time priority** with sub-microsecond
matching latency. A complete trade — buyer meets seller, fill generated, event logged —
executes in **114 nanoseconds**.

---

## Benchmark Results

**Hardware:** 16-core x86_64 · L1=48KB · L2=1280KB · L3=24MB · WSL2 Ubuntu 24.04

| Benchmark | Time | What It Measures |
|-----------|------|-----------------|
| `BM_OrderRest` | 86 ns | Order insert with no match |
| `BM_OrderMatch` | 114 ns | Complete trade execution |
| `BM_RingBuffer` | 7 ns | Lock-free event logging |
| p50 — real workload | 260 ns | Median latency over 100K orders |
| p90 — real workload | 15,156 ns | Heavy multi-fill orders |
| p99 — real workload | 25,643 ns | Worst-case tail latency |
| p99.9 — real workload | 32,281 ns | Extreme tail latency |
| Parse avg | 256 ns | CSV parse per order |
| Throughput | ~137K orders/sec | End-to-end with CSV parsing |
| Trade accuracy | 95,094 / 95,094 | Zero drops through ring buffer |

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                   DATA INGESTION                    │
│      CSV file → parser → MemoryPool<Order>         │
└──────────────────────────┬──────────────────────────┘
                           │ Order*
┌──────────────────────────▼──────────────────────────┐
│                 ORDER BOOK CORE                     │
│   Bid Side                        Ask Side          │
│   vector<PriceLevel>              vector<PriceLevel>│
│   sorted descending               sorted ascending  │
│              ↓     MATCHING ENGINE     ↓            │
│            price-time priority matching             │
└──────────────────────────┬──────────────────────────┘
                           │ TradeEvent
┌──────────────────────────▼──────────────────────────┐
│            LOCK-FREE SPSC RING BUFFER               │
│       atomic head/tail → logger thread             │
└─────────────────────────────────────────────────────┘
```

---

## File Structure

```
nanomatch/
├── CMakeLists.txt             # Build config — Release flags, Google Benchmark
├── README.md
│
├── include/
│   ├── order.h                # alignas(64) Order struct — exactly one cache line
│   ├── price_level.h          # FIFO deque of orders at one price
│   ├── order_book.h           # Full LOB: bid + ask vectors, O(1) cancel
│   ├── matching_engine.h      # Core match loop + ring buffer integration
│   ├── memory_pool.h          # Arena allocator — zero heap on hot path
│   └── ring_buffer.h          # Lock-free SPSC — atomic head/tail
│
├── src/
│   └── main.cpp               # CSV loader, engine runner, latency report
│
├── bench/
│   └── bench_main.cpp         # Google Benchmark suite
│
└── data/synthetic/
    └── gen_orders.py          # Generates N synthetic orders as CSV
```

---

## Why These Decisions

### `alignas(64)` on the Order Struct
Every Order is exactly 64 bytes — one CPU cache line. The CPU fetches memory in
64-byte chunks. If an Order spans two cache lines, the CPU needs two fetches instead
of one — double the latency. `alignas(64)` enforces the boundary.
`static_assert(sizeof(Order) == 64)` makes the compiler verify this at build time.

### Sorted `vector` Instead of `std::map`
`std::map` is a red-black tree. Every node is a separate heap allocation, scattered
in memory. Accessing 10 price levels = 10 pointer chases = 10 potential cache misses.
A sorted `vector` of 10 levels fits in one cache line. Linear scan beats tree traversal
for the small number of active price levels typical in a real order book.

### Memory Pool (`MemoryPool<Order, 200000>`)
`malloc()` takes ~200ns per call — it searches a free-list, marks the block, and
returns a pointer. The pool pre-allocates 200K Order slots in one `aligned_alloc()`
call at startup. `Acquire()` pops a slot in ~5ns. Zero OS calls, zero heap
fragmentation on the hot path.

### Lock-Free SPSC Ring Buffer
A mutex triggers a `futex()` syscall when contended — this costs 1–10 microseconds,
longer than the entire match cycle. The SPSC ring buffer uses `std::atomic` head and
tail indices with `memory_order_release` / `memory_order_acquire` ordering. No kernel
involvement. No blocking. Both indices have their own `alignas(64)` cache line to
prevent false sharing between the producer (engine) and consumer (logger) cores.

### Profiling Result
Callgrind revealed that **44% of CPU instructions in benchmarks were heap allocator
calls** — confirming the memory pool is the single highest-impact optimisation for
production workloads. Only 8% of instructions were actual matching logic.

---

## Key Engineering Challenges

**1. `assert()` silently disabled in Release builds**
A quantity conservation check using `assert()` appeared to pass silently while the
numbers showed a mismatch. Root cause: CMake Release mode passes `-DNDEBUG`, which
strips all `assert()` calls at compile time. Fixed by replacing with explicit
`if`-checks that work in all build modes.

**2. Const correctness compiler error**
`BestBid()` / `BestAsk()` were called on a `const OrderBook&`. The compiler refused
because `Bids()` and `Asks()` were not marked `const`. Fixed by adding `const`
overloads — the compiler picks the correct version based on the object's
const-qualification.

**3. False sharing in the ring buffer**
`head_` and `tail_` declared sequentially shared a cache line. The producer writing
`tail_` invalidated the consumer's cached copy of `head_` on every push — constant
cross-core cache-line transfer. Fixed with `alignas(64)` on both, giving each its
own dedicated cache line.

**4. Ghost price levels after cancel**
Cancelling the last order at a price left an empty `PriceLevel` in the vector.
`BestBid()` returned this empty level, causing a null dereference downstream.
Fixed with the erase-remove idiom using `std::remove_if` after every cancel.

---

## Concepts Demonstrated

- CPU cache hierarchy and cache line alignment
- Custom arena memory allocator (placement new, free-list)
- Lock-free concurrency with `std::atomic` and memory ordering
- Price-time priority matching algorithm
- SPSC ring buffer for producer-consumer decoupling
- Callgrind profiling and bottleneck identification
- Google Benchmark for statistical latency measurement (`p50` / `p90` / `p99`)
- CMake build system with FetchContent dependency management

---

## How to Build and Run

**Requirements:** g++ 12+, cmake 3.20+, Linux or WSL2

```bash
# Clone
git clone https://github.com/babbyy-shark/nanomatch.git
cd nanomatch

# Build (downloads Google Benchmark automatically)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Generate 100K synthetic orders
python3 data/synthetic/gen_orders.py --orders 100000 --output orders.csv

# Run the engine — prints p50/p90/p99 latency breakdown
./build/nanomatch data/synthetic/orders.csv

# Run Google Benchmark suite
./build/nanomatch_bench
```
#pragma once
#include <cstdint>

enum class Side : uint8_t { BUY = 0, SELL = 1 };
enum class OrderType : uint8_t { LIMIT = 0, MARKET = 1 };

struct alignas(64) Order {
    uint64_t  order_id;
    int64_t   price;      // in ticks — never use float for money
    uint32_t  quantity;
    uint32_t  remaining;
    Side      side;
    OrderType type;
    uint8_t   _pad[38];   // pad to exactly 64 bytes
};

static_assert(sizeof(Order) == 64, "Order must be exactly one cache line");

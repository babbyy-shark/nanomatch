#pragma once
#include <vector>
#include "order_book.h"

struct TradeEvent {
    uint64_t aggressor_id;
    uint64_t resting_id;
    int64_t  price;
    uint32_t quantity;
};

class MatchingEngine {
public:
    MatchingEngine() { trades_.reserve(500000); }

    void ProcessOrder(Order* order) {
        if (order->type == OrderType::LIMIT)
            ProcessLimit(order);
        else
            ProcessMarket(order);
    }

    const std::vector<TradeEvent>& Trades() const { return trades_; }
    const OrderBook& Book() const { return book_; }

private:
    void ProcessLimit(Order* incoming) {
        while (incoming->remaining > 0) {
            PriceLevel* best = (incoming->side == Side::BUY)
                ? book_.BestAsk() : book_.BestBid();
            if (!best) break;

            bool crosses = (incoming->side == Side::BUY)
                ? incoming->price >= best->price
                : incoming->price <= best->price;
            if (!crosses) break;

            while (incoming->remaining > 0 && !best->orders.empty()) {
                Order* resting = best->orders.front();
                uint32_t fill  = std::min(incoming->remaining,
                                          resting->remaining);
                trades_.push_back({incoming->order_id,
                                   resting->order_id,
                                   best->price, fill});
                incoming->remaining -= fill;
                resting->remaining  -= fill;
                if (resting->remaining == 0)
                    best->orders.pop_front();
            }
        }
        if (incoming->remaining > 0) book_.Add(incoming);
    }

    void ProcessMarket(Order* incoming) {
        while (incoming->remaining > 0) {
            PriceLevel* best = (incoming->side == Side::BUY)
                ? book_.BestAsk() : book_.BestBid();
            if (!best) break;

            while (incoming->remaining > 0 && !best->orders.empty()) {
                Order* resting = best->orders.front();
                uint32_t fill  = std::min(incoming->remaining,
                                          resting->remaining);
                trades_.push_back({incoming->order_id,
                                   resting->order_id,
                                   best->price, fill});
                incoming->remaining -= fill;
                resting->remaining  -= fill;
                if (resting->remaining == 0)
                    best->orders.pop_front();
            }
        }
    }

    OrderBook book_;
    std::vector<TradeEvent> trades_;
};

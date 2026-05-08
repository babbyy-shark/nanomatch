#pragma once
#include <vector>
#include "order_book.h"

struct TradeEvent {
    uint64_t aggressor_id;  // incoming order that triggered the match
    uint64_t resting_id;    // order sitting in the book
    int64_t  price;         // fill price (resting order's price)
    uint32_t quantity;      // how many shares filled
};

class MatchingEngine {
public:
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
        auto& opposing = (incoming->side == Side::BUY)
            ? book_.Asks() : book_.Bids();

        while (incoming->remaining > 0 && !opposing.empty()) {
            PriceLevel& best = opposing.front();

            // Price check: does this order cross the spread?
            bool crosses = (incoming->side == Side::BUY)
                ? incoming->price >= best.price
                : incoming->price <= best.price;
            if (!crosses) break;

            // Match against each resting order at this level (FIFO)
            while (incoming->remaining > 0 && !best.orders.empty()) {
                Order* resting = best.orders.front();
                uint32_t fill = std::min(incoming->remaining,
                                         resting->remaining);

                trades_.push_back(TradeEvent{
                    incoming->order_id,
                    resting->order_id,
                    best.price,
                    fill
                });

                incoming->remaining -= fill;
                resting->remaining  -= fill;

                if (resting->remaining == 0) {
                    best.orders.pop_front();
                }
            }
            if (best.orders.empty()) opposing.erase(opposing.begin());
        }

        // Rest remaining quantity in the book
        if (incoming->remaining > 0) book_.Add(incoming);
    }

    void ProcessMarket(Order* incoming) {
        auto& opposing = (incoming->side == Side::BUY)
            ? book_.Asks() : book_.Bids();

        while (incoming->remaining > 0 && !opposing.empty()) {
            PriceLevel& best = opposing.front();

            while (incoming->remaining > 0 && !best.orders.empty()) {
                Order* resting = best.orders.front();
                uint32_t fill = std::min(incoming->remaining,
                                         resting->remaining);

                trades_.push_back(TradeEvent{
                    incoming->order_id,
                    resting->order_id,
                    best.price,
                    fill
                });

                incoming->remaining -= fill;
                resting->remaining  -= fill;

                if (resting->remaining == 0)
                    best.orders.pop_front();
            }
            if (best.orders.empty()) opposing.erase(opposing.begin());
        }
        // Market orders don't rest — if unfilled, they're dropped
    }

    OrderBook book_;
    std::vector<TradeEvent> trades_;
};

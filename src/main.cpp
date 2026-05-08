#include <iostream>
#include <cassert>
#include "order.h"
#include "order_book.h"
#include "matching_engine.h"

int main() {
    std::cout << "NANOMATCH booting...\n\n";

    MatchingEngine engine;
    uint32_t total_in = 0;

    Order sells[10], buys[10];
    for (int i = 0; i < 10; i++) {
        sells[i] = {(uint64_t)(100+i), (int64_t)(10000 + i*5),
                    (uint32_t)(50+i*10), (uint32_t)(50+i*10),
                    Side::SELL, OrderType::LIMIT, {}};
        total_in += sells[i].quantity;
        engine.ProcessOrder(&sells[i]);
    }
    for (int i = 0; i < 10; i++) {
        buys[i] = {(uint64_t)(200+i), (int64_t)(10050 - i*3),
                   (uint32_t)(40+i*15), (uint32_t)(40+i*15),
                   Side::BUY, OrderType::LIMIT, {}};
        total_in += buys[i].quantity;
        engine.ProcessOrder(&buys[i]);
    }

    uint32_t total_filled = 0;
    for (auto& t : engine.Trades())
        total_filled += t.quantity;

    uint32_t total_resting = 0;
    for (auto& level : engine.Book().Bids())
        for (auto* o : level.orders)
            total_resting += o->remaining;
    for (auto& level : engine.Book().Asks())
        for (auto* o : level.orders)
            total_resting += o->remaining;

    std::cout << "Total shares in:      " << total_in      << "\n";
    std::cout << "Total shares filled:  " << total_filled  << "\n";
    std::cout << "Total shares resting: " << total_resting << "\n";
    std::cout << "Accounted for:        " << total_filled*2 + total_resting << "\n";

    // Each fill consumes from TWO orders (buyer + seller) both counted in total_in
    // So: total_filled*2 + total_resting == total_in
    if (total_filled * 2 + total_resting != total_in) {
        std::cerr << "CONSERVATION FAILED — shares lost!\n";
        return 1;
    }

    std::cout << "\nQuantity conservation: PASSED (real check)\n";
    std::cout << "Trades generated:     " << engine.Trades().size() << "\n";
    std::cout << "\nDay 6 PASSED\n";
    return 0;
}

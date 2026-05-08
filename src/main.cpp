#include <iostream>
#include <cassert>
#include "order.h"
#include "order_book.h"
#include "matching_engine.h"

int main() {
    std::cout << "Order size: " << sizeof(Order) << " bytes\n";
    std::cout << "Cache aligned: " << (alignof(Order) == 64 ? "YES" : "NO") << "\n";
    std::cout << "NANOMATCH booting...\n\n";

    MatchingEngine engine;

    // Add resting SELL orders to the book
    Order s1{1, 10045, 100, 100, Side::SELL, OrderType::LIMIT, {}};
    Order s2{2, 10050,  50,  50, Side::SELL, OrderType::LIMIT, {}};
    engine.ProcessOrder(&s1);
    engine.ProcessOrder(&s2);

    // No trades yet — these just rest in the book
    assert(engine.Trades().size() == 0);
    std::cout << "Resting orders added: PASSED\n";

    // BUY at 10050 — should match s1 fully (100 shares at 10045)
    // then match s2 partially (needs 50 more, s2 has exactly 50)
    Order b1{3, 10050, 150, 150, Side::BUY, OrderType::LIMIT, {}};
    engine.ProcessOrder(&b1);

    assert(engine.Trades().size() == 2);

    // First trade: matched s1 at 10045 for 100 shares
    assert(engine.Trades()[0].resting_id  == 1);
    assert(engine.Trades()[0].price       == 10045);
    assert(engine.Trades()[0].quantity    == 100);

    // Second trade: matched s2 at 10050 for 50 shares
    assert(engine.Trades()[1].resting_id  == 2);
    assert(engine.Trades()[1].price       == 10050);
    assert(engine.Trades()[1].quantity    == 50);

    // b1 fully filled — should NOT be in book
    assert(engine.Book().BestAsk() == nullptr);
    std::cout << "Limit order match: PASSED\n";

    // Market order test — add a resting SELL, fire market BUY
    MatchingEngine engine2;
    Order s3{10, 10060, 200, 200, Side::SELL, OrderType::LIMIT,  {}};
    Order m1{11,     0,  80,  80, Side::BUY,  OrderType::MARKET, {}};
    engine2.ProcessOrder(&s3);
    engine2.ProcessOrder(&m1);

    assert(engine2.Trades().size() == 1);
    assert(engine2.Trades()[0].quantity == 80);
    assert(engine2.Trades()[0].price    == 10060);
    std::cout << "Market order match: PASSED\n";

    // Partial fill — BUY more than available
    MatchingEngine engine3;
    Order s4{20, 10040, 50, 50, Side::SELL, OrderType::LIMIT, {}};
    Order b2{21, 10040, 80, 80, Side::BUY,  OrderType::LIMIT, {}};
    engine3.ProcessOrder(&s4);
    engine3.ProcessOrder(&b2);

    assert(engine3.Trades().size() == 1);
    assert(engine3.Trades()[0].quantity == 50);  // only 50 available
    // Remaining 30 should rest as BUY in book
    assert(engine3.Book().BestBid()->price    == 10040);
    assert(engine3.Book().BestBid()->orders.front()->remaining == 30);
    std::cout << "Partial fill: PASSED\n";

    std::cout << "\nAll Day 5 tests PASSED — first trades generated!\n";
    return 0;
}

#include <iostream>
#include <cassert>
#include "order.h"
#include "price_level.h"
#include "order_book.h"

int main() {
    std::cout << "Order size: " << sizeof(Order) << " bytes\n";
    std::cout << "Cache aligned: " << (alignof(Order) == 64 ? "YES" : "NO") << "\n";
    std::cout << "NANOMATCH booting...\n\n";

    // Days 2-3 regression check
    OrderBook book;
    Order o1{10, 10050, 100, 100, Side::BUY,  OrderType::LIMIT, {}};
    Order o2{11, 10045, 200, 200, Side::BUY,  OrderType::LIMIT, {}};
    Order o3{12, 10055,  50,  50, Side::BUY,  OrderType::LIMIT, {}};
    Order o4{20, 10060, 150, 150, Side::SELL, OrderType::LIMIT, {}};
    Order o5{21, 10065,  75,  75, Side::SELL, OrderType::LIMIT, {}};

    book.Add(&o1); book.Add(&o2); book.Add(&o3);
    book.Add(&o4); book.Add(&o5);

    assert(book.BestBid()->price == 10055);
    assert(book.BestAsk()->price == 10060);
    assert(book.BidLevels() == 3);
    assert(book.AskLevels() == 2);
    std::cout << "Basic book: PASSED\n";

    // Day 4 — cancel the ONLY order at a price level
    // Cancelling o3 (only order at 10055) should remove that level entirely
    assert(book.Cancel(12) == true);
    assert(book.BidLevels() == 2);           // level at 10055 gone
    assert(book.BestBid()->price == 10050);  // best bid drops to 10050
    std::cout << "Empty level cleanup: PASSED\n";

    // Cancel non-existent order
    assert(book.Cancel(999) == false);
    std::cout << "Cancel non-existent: PASSED\n";

    // Cancel all orders on ask side — book should report empty asks
    assert(book.Cancel(20) == true);
    assert(book.Cancel(21) == true);
    assert(book.BestAsk() == nullptr);       // no asks left
    assert(book.AskLevels() == 0);
    std::cout << "Full side cancel: PASSED\n";

    // Two orders at same price — cancel one, other remains
    Order o6{30, 10050, 100, 100, Side::BUY, OrderType::LIMIT, {}};
    book.Add(&o6);
    assert(book.Cancel(10) == true);         // cancel o1
    assert(book.BestBid()->price == 10050);  // level still exists (o6 remains)
    assert(book.BidLevels() == 2);           // 10050 and 10045 still there
    std::cout << "Partial level cancel: PASSED\n";

    std::cout << "\nAll Day 4 tests PASSED\n";
    return 0;
}

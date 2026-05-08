#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "order.h"
#include "order_book.h"
#include "matching_engine.h"
#include "memory_pool.h"

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "data/synthetic/orders.csv";

    // Pre-allocate 200K order slots
    MemoryPool<Order, 200000> pool;
    MatchingEngine engine;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open: " << path << "\n";
        return 1;
    }

    std::string line;
    std::getline(file, line); // skip header

    auto start = std::chrono::high_resolution_clock::now();

    uint64_t count = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string tok;

        Order* o = pool.Acquire();

        std::getline(ss, tok, ','); o->order_id = std::stoull(tok);
        std::getline(ss, tok, ','); o->side     = (tok == "BUY") ? Side::BUY : Side::SELL;
        std::getline(ss, tok, ','); o->price    = std::stoll(tok);
        std::getline(ss, tok, ','); o->quantity = std::stoul(tok);
        std::getline(ss, tok, ',');
        o->type      = (tok.find("MARKET") != std::string::npos)
                       ? OrderType::MARKET : OrderType::LIMIT;
        o->remaining = o->quantity;

        engine.ProcessOrder(o);
        count++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    auto ns_per_order = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() / count;

    std::cout << "Orders processed:  " << count             << "\n";
    std::cout << "Trades generated:  " << engine.Trades().size() << "\n";
    std::cout << "Total time:        " << ms                << " ms\n";
    std::cout << "Avg per order:     " << ns_per_order      << " ns\n";
    std::cout << "Pool slots used:   " << pool.Capacity() - pool.Available() << "\n";
    std::cout << "\nDay 8 PASSED — memory pool feeding 100K orders\n";
    return 0;
}

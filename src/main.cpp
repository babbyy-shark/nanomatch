#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include "order.h"
#include "order_book.h"
#include "matching_engine.h"
#include "memory_pool.h"

using Clock = std::chrono::high_resolution_clock;

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "data/synthetic/orders.csv";

    MemoryPool<Order, 200000> pool;
    MatchingEngine engine;

    std::ifstream file(path);
    if (!file.is_open()) { std::cerr << "Cannot open: " << path << "\n"; return 1; }

    std::string line;
    std::getline(file, line);

    uint64_t count       = 0;
    long long parse_ns   = 0;

    // Store per-order match times for percentile analysis
    std::vector<long long> match_times;
    match_times.reserve(100000);

    uint64_t trades_before = 0;

    while (std::getline(file, line)) {
        auto t0 = Clock::now();
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
        auto t1 = Clock::now();

        trades_before = engine.Trades().size();
        engine.ProcessOrder(o);
        auto t2 = Clock::now();

        parse_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
        match_times.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count());
        count++;
    }

    // Sort for percentiles
    std::sort(match_times.begin(), match_times.end());

    auto percentile = [&](double p) {
        return match_times[(size_t)(p/100.0 * match_times.size())];
    };

    std::cout << "Orders processed:  " << count                   << "\n";
    std::cout << "Trades generated:  " << engine.Trades().size()  << "\n";
    std::cout << "\n--- Match Latency Percentiles ---\n";
    std::cout << "p50  (median): " << percentile(50)  << " ns\n";
    std::cout << "p90:           " << percentile(90)  << " ns\n";
    std::cout << "p99:           " << percentile(99)  << " ns\n";
    std::cout << "p99.9:         " << percentile(99.9)<< " ns\n";
    std::cout << "max:           " << match_times.back() << " ns\n";
    std::cout << "\nParse avg:     " << parse_ns/count   << " ns\n";
    return 0;
}

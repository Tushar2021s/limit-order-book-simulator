#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "order.h"

#include <vector>
#include <deque>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <chrono>

class OrderBook {
private:
    static const int MAX_BUCKETS = 100000;  // max price points

    std::vector<std::deque<Order>> bids;   // index = price bucket
    std::vector<std::deque<Order>> asks;

    double minPrice;
    double maxPrice;
    double priceMultiplier; // maps price -> index

    // OrderID -> {bucketIndex, positionInDeque}
    std::unordered_map<int, std::pair<int,int>> orderLookup;

    std::vector<Trade> tradeHistory;
    std::ofstream logFile;
    long long ordersProcessed = 0;

    std::chrono::high_resolution_clock::time_point benchmarkStart;

    int priceToIndex(double price) const {
        return static_cast<int>((price - minPrice) * priceMultiplier);
    }

public:
    OrderBook(double minP = 0.0, double maxP = 1000.0, double multiplier = 100.0);

    void addLimitOrder(const Order& order);
    void cancelOrder(int id);
    void match();
    void executeMarketOrder(Side side, int quantity);

    void printBook() const;
    void printTrades() const;
    void printThroughput() const;

    // Non-copyable
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    ~OrderBook();
};

#endif






















// #ifndef ORDER_BOOK_H
// #define ORDER_BOOK_H

// #include "order.h"

// #include <map>
// #include <list>
// #include <unordered_map>
// #include <vector>
// #include <fstream>
// #include <iostream>

// class OrderBook {

// private:

//     // BUY orders (highest price first)
//     std::map<double, std::list<Order>, std::greater<double>> bids;

//     // SELL orders (lowest price first)
//     std::map<double, std::list<Order>> asks;

//     // O(1) lookup: OrderID -> iterator to order
//     std::unordered_map<int, std::list<Order>::iterator> orderLookup;

//     // Trade history storage
//     std::vector<Trade> tradeHistory;
//     std::ofstream logFile;
//     long long ordersProcessed = 0;

//     std::chrono::high_resolution_clock::time_point benchmarkStart;

// public:

//     void addLimitOrder(const Order& order);

//     void cancelOrder(int id);

//     void match();

//     void printBook();

//     void printTrades();
//     void printThroughput();
//     void executeMarketOrder(Side side, int quantity);
//     OrderBook() {
//         logFile.open("latency_log.csv");

//         if (!logFile.is_open()) {
//             std::cerr << "Error opening latency_log.csv\n";
//         }

//         logFile << "order_id,network_us,queue_ns,engine_ns,total_us\n";
//          logFile.tie(nullptr);  //for faster logging
//     }
//     ~OrderBook() {
//         if (logFile.is_open()) {
//             logFile.close();
//         }
//     }
//     OrderBook(const OrderBook&) = delete;
//     OrderBook& operator=(const OrderBook&) = delete;
// };

// #endif


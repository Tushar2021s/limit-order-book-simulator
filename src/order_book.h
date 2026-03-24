#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "order.h"

#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iostream>

class OrderBook {

private:

    // BUY orders (highest price first)
    std::map<double, std::list<Order>, std::greater<double>> bids;

    // SELL orders (lowest price first)
    std::map<double, std::list<Order>> asks;

    // O(1) lookup: OrderID -> iterator to order
    std::unordered_map<int, std::list<Order>::iterator> orderLookup;

    // Trade history storage
    std::vector<Trade> tradeHistory;
    std::ofstream logFile;
    long long ordersProcessed = 0;

    std::chrono::high_resolution_clock::time_point benchmarkStart;

public:

    void addLimitOrder(const Order& order);

    void cancelOrder(int id);

    void match();

    void printBook();

    void printTrades();
    void printThroughput();
    void executeMarketOrder(Side side, int quantity);
    OrderBook() {
        logFile.open("latency_log.csv");

        if (!logFile.is_open()) {
            std::cerr << "Error opening latency_log.csv\n";
        }

        logFile << "order_id,network_us,queue_ns,engine_ns,total_us\n";
         logFile.tie(nullptr);  //for faster logging
    }
    ~OrderBook() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
};

#endif


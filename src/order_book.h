#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "order.h"

#include <map>
#include <list>
#include <unordered_map>
#include <vector>

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

public:

    void addLimitOrder(const Order& order);

    void cancelOrder(int id);

    void match();

    void printBook();

    void printTrades();

    void executeMarketOrder(Side side, int quantity);
};

#endif


// #ifndef ORDER_BOOK_H
// #define ORDER_BOOK_H

// #include "order.h"
// #include <map>
// #include <deque>
// #include <list>
// #include <unordered_map>
// #include <vector>
// class OrderBook {

// private:

//     std::map<double, std::deque<Order>, std::greater<double>> bids;
//     std::map<double, std::deque<Order>> asks;
//     std::unordered_map<int, std::list<Order>::iterator> orderLookup;
//     std::vector<Trade> tradeHistory;

// public:

//     void addLimitOrder(const Order& order);
//     void cancelOrder(int id);
//     void match();
//     void printBook();
//     void printTrades();
//     void executeMarketOrder(Side side, int quantity);

// };

// struct PriceLevel {
//     std::list<Order> orders;
// };

// #endif
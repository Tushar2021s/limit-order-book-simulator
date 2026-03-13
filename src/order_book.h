#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "order.h"
#include <map>
#include <deque>

class OrderBook {

private:

    std::map<double, std::deque<Order>, std::greater<double>> bids;
    std::map<double, std::deque<Order>> asks;

public:

    void addLimitOrder(const Order& order);

    void match();

    void printBook();

};

#endif
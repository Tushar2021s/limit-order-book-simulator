#ifndef ORDER_H
#define ORDER_H

#include <string>

enum class OrderType {
    LIMIT,
    MARKET
};

enum class Side {
    BUY,
    SELL
};

struct Order {
    int id;
    Side side;
    double price;
    int quantity;
    OrderType type;
};

#endif
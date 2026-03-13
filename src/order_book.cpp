#include "order_book.h"
#include <iostream>

void OrderBook::addLimitOrder(const Order& order) {

    if(order.side == Side::BUY) {
        bids[order.price].push_back(order);
    }
    else {
        asks[order.price].push_back(order);
    }

}
void OrderBook::printBook() {

    std::cout << "----- ORDER BOOK -----\n";

    std::cout << "\nSELL ORDERS\n";
    for(auto &level : asks) {
        std::cout << "Price: " << level.first
                  << " Qty: " << level.second.size() << "\n";
    }

    std::cout << "\nBUY ORDERS\n";
    for(auto &level : bids) {
        std::cout << "Price: " << level.first
                  << " Qty: " << level.second.size() << "\n";
    }
}
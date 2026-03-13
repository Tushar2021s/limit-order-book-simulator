#include "order_book.h"
#include <iostream>

void OrderBook::addLimitOrder(const Order& order) {

    if(order.side == Side::BUY)
        bids[order.price].push_back(order);
    else
        asks[order.price].push_back(order);

    match();
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
void OrderBook::match() {

    while(!bids.empty() && !asks.empty()) {

        auto bestBid = bids.begin();
        auto bestAsk = asks.begin();

        double bidPrice = bestBid->first;
        double askPrice = bestAsk->first;

        if(bidPrice < askPrice)
            break;

        Order &buyOrder = bestBid->second.front();
        Order &sellOrder = bestAsk->second.front();

        int tradeQty = std::min(buyOrder.quantity, sellOrder.quantity);

        std::cout << "TRADE EXECUTED: "
                  << tradeQty
                  << " @ "
                  << askPrice
                  << "\n";

        buyOrder.quantity -= tradeQty;
        sellOrder.quantity -= tradeQty;

        if(buyOrder.quantity == 0)
            bestBid->second.pop_front();

        if(sellOrder.quantity == 0)
            bestAsk->second.pop_front();

        if(bestBid->second.empty())
            bids.erase(bestBid);

        if(bestAsk->second.empty())
            asks.erase(bestAsk);
    }
}
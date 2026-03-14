#include "order_book.h"
#include <iostream>
#include <chrono>
#include <thread>

#include <random>
#include <ctime>
std::string getCurrentTime() {

    std::time_t now = std::time(nullptr);
    char buffer[80];

    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));

    return std::string(buffer);
}
// void OrderBook::addLimitOrder(const Order& order) {

//     if(order.side == Side::BUY)
//         bids[order.price].push_back(order);
//     else
//         asks[order.price].push_back(order);

//     match();
// }before latency upgrade

// void OrderBook::addLimitOrder(const Order& order) {

//     auto start = std::chrono::high_resolution_clock::now();

//     if(order.side == Side::BUY)
//         bids[order.price].push_back(order);
//     else
//         asks[order.price].push_back(order);

//     match();

//     auto end = std::chrono::high_resolution_clock::now();

//     auto latency =
//         std::chrono::duration_cast<std::chrono::microseconds>(end - start);

//     std::cout << "Matching Engine Latency: "
//               << latency.count()
//               << " microseconds\n";
// }before upgrading network latency 
// void OrderBook::addLimitOrder(const Order& order) {

//     auto start = std::chrono::high_resolution_clock::now();

//     // Simulate network latency
//     std::this_thread::sleep_for(std::chrono::microseconds(50));

//     if(order.side == Side::BUY)
//         bids[order.price].push_back(order);
//     else
//         asks[order.price].push_back(order);

//     match();

//     auto end = std::chrono::high_resolution_clock::now();

//     auto latency =
//         std::chrono::duration_cast<std::chrono::microseconds>(end - start);

//     std::cout << "Total Order Processing Latency: "
//               << latency.count()
//               << " microseconds\n";
// }before random latency
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dist(20,100);
void OrderBook::addLimitOrder(const Order& order) {

    auto start = std::chrono::high_resolution_clock::now();

    int networkDelay = dist(gen);

    std::cout << "Network Latency: "
              << networkDelay
              << " microseconds\n";

    std::this_thread::sleep_for(
        std::chrono::microseconds(networkDelay)
    );

    if(order.side == Side::BUY) {

        bids[order.price].push_back(order);

        auto it = bids[order.price].end();
        --it;

        orderLookup[order.id] = it;
    }
    else {

        asks[order.price].push_back(order);

        auto it = asks[order.price].end();
        --it;

        orderLookup[order.id] = it;
    }

    match();

    auto end = std::chrono::high_resolution_clock::now();

    auto totalLatency =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Total Processing Latency: "
              << totalLatency.count()
              << " microseconds\n";
}
// void OrderBook::addLimitOrder(const Order& order) {

//     auto start = std::chrono::high_resolution_clock::now();

//     // Random latency generator
//     // std::random_device rd;
//     // std::mt19937 gen(rd());
//     // std::uniform_int_distribution<> dist(20,100);

//     int networkDelay = dist(gen);

//     std::cout << "Network Latency: "
//               << networkDelay
//               << " microseconds\n";

//     std::this_thread::sleep_for(
//         std::chrono::microseconds(networkDelay)
//     );

//     if(order.side == Side::BUY)
//         bids[order.price].push_back(order);
//     else
//         asks[order.price].push_back(order);

//     match();

//     auto end = std::chrono::high_resolution_clock::now();

//     auto totalLatency =
//         std::chrono::duration_cast<std::chrono::microseconds>(end - start);

//     std::cout << "Total Processing Latency: "
//               << totalLatency.count()
//               << " microseconds\n";
// }
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
// void OrderBook::match() {

//     while(!bids.empty() && !asks.empty()) {

//         auto bestBid = bids.begin();
//         auto bestAsk = asks.begin();

//         double bidPrice = bestBid->first;
//         double askPrice = bestAsk->first;

//         if(bidPrice < askPrice)
//             break;

//         Order &buyOrder = bestBid->second.front();
//         Order &sellOrder = bestAsk->second.front();

//         int tradeQty = std::min(buyOrder.quantity, sellOrder.quantity);
//         Trade trade;

//         trade.buyOrderId = buyOrder.id;
//         trade.sellOrderId = sellOrder.id;
//         trade.price = askPrice;
//         trade.quantity = tradeQty;
//         trade.timestamp = getCurrentTime();

//         tradeHistory.push_back(trade);

//         std::cout << "TRADE EXECUTED: "
//                   << tradeQty
//                   << " @ "
//                   << askPrice
//                   << "\n";

//         buyOrder.quantity -= tradeQty;
//         sellOrder.quantity -= tradeQty;

//         if(buyOrder.quantity == 0)
//             bestBid->second.pop_front();

//         if(sellOrder.quantity == 0)
//             bestAsk->second.pop_front();

//         if(bestBid->second.empty())
//             bids.erase(bestBid);

//         if(bestAsk->second.empty())
//             asks.erase(bestAsk);
//     }
// }
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

        Trade trade;
        trade.buyOrderId = buyOrder.id;
        trade.sellOrderId = sellOrder.id;
        trade.price = askPrice;
        trade.quantity = tradeQty;
        trade.timestamp = getCurrentTime();

        tradeHistory.push_back(trade);

        std::cout << "TRADE EXECUTED: "
                  << tradeQty
                  << " @ "
                  << askPrice
                  << "\n";

        buyOrder.quantity -= tradeQty;
        sellOrder.quantity -= tradeQty;

        if(buyOrder.quantity == 0) {

            orderLookup.erase(buyOrder.id);   // remove from lookup table
            bestBid->second.pop_front();
        }

        if(sellOrder.quantity == 0) {

            orderLookup.erase(sellOrder.id);  // remove from lookup table
            bestAsk->second.pop_front();
        }

        if(bestBid->second.empty())
            bids.erase(bestBid);

        if(bestAsk->second.empty())
            asks.erase(bestAsk);
    }
}
// void OrderBook::cancelOrder(int id) {

//     auto it = orderLookup.find(id);

//     if(it == orderLookup.end()) {
//         std::cout << "Order not found\n";
//         return;
//     }

//     auto orderIt = it->second;

//     orderIt->quantity = 0;

//     orderLookup.erase(it);
// }
void OrderBook::cancelOrder(int id) {

    auto it = orderLookup.find(id);

    if(it == orderLookup.end()) {

        std::cout << "Order not found\n";
        return;
    }

    auto orderIt = it->second;

    std::cout << "Order "
              << orderIt->id
              << " cancelled\n";

    orderIt->quantity = 0;

    orderLookup.erase(it);
}
void OrderBook::printTrades() {

    std::cout << "\n--- TRADE HISTORY ---\n";

    for(const auto &trade : tradeHistory) {

        std::cout << "BUY:"
                  << trade.buyOrderId
                  << " SELL:"
                  << trade.sellOrderId
                  << " PRICE:"
                  << trade.price
                  << " QTY:"
                  << trade.quantity
                  << " TIME:"
                  << trade.timestamp
                  << "\n";
    }
}
void OrderBook::executeMarketOrder(Side side, int quantity) {

    if(side == Side::BUY) {

        while(quantity > 0 && !asks.empty()) {

            auto bestAsk = asks.begin();
            double askPrice = bestAsk->first;

            Order &sellOrder = bestAsk->second.front();

            int tradeQty = std::min(quantity, sellOrder.quantity);

            std::cout << "MARKET TRADE: "
                      << tradeQty
                      << " @ "
                      << askPrice
                      << "\n";

            sellOrder.quantity -= tradeQty;
            quantity -= tradeQty;

            if(sellOrder.quantity == 0)
                bestAsk->second.pop_front();

            if(bestAsk->second.empty())
                asks.erase(bestAsk);
        }
    }

    else {

        while(quantity > 0 && !bids.empty()) {

            auto bestBid = bids.begin();
            double bidPrice = bestBid->first;

            Order &buyOrder = bestBid->second.front();

            int tradeQty = std::min(quantity, buyOrder.quantity);

            std::cout << "MARKET TRADE: "
                      << tradeQty
                      << " @ "
                      << bidPrice
                      << "\n";

            buyOrder.quantity -= tradeQty;
            quantity -= tradeQty;

            if(buyOrder.quantity == 0)
                bestBid->second.pop_front();

            if(bestBid->second.empty())
                bids.erase(bestBid);
        }
    }

    if(quantity > 0)
        std::cout << "Unfilled quantity: " << quantity << "\n";
}
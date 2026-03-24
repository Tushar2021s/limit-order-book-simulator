#include "order_book.h"
#include <iostream>
#include <chrono>
#include <thread>

#include <random>
#include <ctime>
#include <fstream>

std::ofstream logFile("latency_log.csv");
std::string getCurrentTime() {

    std::time_t now = std::time(nullptr);
    char buffer[80];

    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));

    return std::string(buffer);
}

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dist(20,100);
void OrderBook::addLimitOrder(const Order& order) {

    auto startTotal = std::chrono::high_resolution_clock::now();
     ordersProcessed++;

    // ---------------- NETWORK DELAY ----------------
    int networkDelay = dist(gen);

    std::this_thread::sleep_for(
        std::chrono::microseconds(networkDelay)
    );

    // ---------------- COPY ORDER ----------------
    Order newOrder = order;

    // ---------------- QUEUE START ----------------
    auto queueStart = std::chrono::high_resolution_clock::now();

    // timestamp (AFTER network delay)
    newOrder.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        queueStart.time_since_epoch()
    ).count();

    // ---------------- INSERT INTO BOOK ----------------
    if(newOrder.side == Side::BUY) {

        bids[newOrder.price].push_back(newOrder);
        auto it = std::prev(bids[newOrder.price].end());
        orderLookup[newOrder.id] = it;

    } else {

        asks[newOrder.price].push_back(newOrder);
        auto it = std::prev(asks[newOrder.price].end());
        orderLookup[newOrder.id] = it;
    }

    // ---------------- QUEUE END ----------------
    auto queueEnd = std::chrono::high_resolution_clock::now();

    auto queuingDelay =
        std::chrono::duration_cast<std::chrono::nanoseconds>(queueEnd - queueStart).count();

    // ---------------- MATCHING ENGINE ----------------
    auto engineStart = std::chrono::high_resolution_clock::now();

    match();

    auto engineEnd = std::chrono::high_resolution_clock::now();

    auto engineLatency =
        std::chrono::duration_cast<std::chrono::nanoseconds>(engineEnd - engineStart);

    // ---------------- TOTAL LATENCY ----------------
    auto endTotal = std::chrono::high_resolution_clock::now();

    auto totalLatency =
        std::chrono::duration_cast<std::chrono::microseconds>(endTotal - startTotal);

    // ---------------- PRINT (DEBUG) ----------------
    std::cout << "Network Latency: " << networkDelay << " us\n";
    std::cout << "Queuing Delay: " << queuingDelay << " ns\n";
    std::cout << "Engine Latency: " << engineLatency.count() << " ns\n";
    std::cout << "Total Latency: " << totalLatency.count() << " us\n";

    // ---------------- CSV LOGGING ----------------
    logFile << newOrder.id << ","
            << networkDelay << ","
            << queuingDelay << ","
            << engineLatency.count() << ","
            << totalLatency.count()
            << "\n";
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

void OrderBook::cancelOrder(int id) {
    auto it = orderLookup.find(id);
    if (it == orderLookup.end()) return;

    auto orderIt = it->second;
    auto price = orderIt->price;

    if (orderIt->side == Side::BUY) {
        auto &level = bids[price];
        level.erase(orderIt);
        if (level.empty()) bids.erase(price);
    } else {
        auto &level = asks[price];
        level.erase(orderIt);
        if (level.empty()) asks.erase(price);
    }

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
OrderBook::OrderBook() {
    benchmarkStart = std::chrono::high_resolution_clock::now();
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
void OrderBook::printThroughput() {

    auto benchmarkEnd = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        benchmarkEnd - benchmarkStart
    ).count();

    long long throughput = ordersProcessed / std::max(1LL, duration);

    std::cout << "Orders Processed: " << ordersProcessed << "\n";
    std::cout << "Time Taken: " << duration << " sec\n";
    std::cout << "Throughput: " << throughput << " orders/sec\n";
}
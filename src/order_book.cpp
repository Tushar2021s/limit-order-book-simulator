#include "order_book.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <ctime>
#include <deque>

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<int> dist100(0, 99);
static std::uniform_int_distribution<int> fastDist(0, 49);    // fast
static std::uniform_int_distribution<int> medDist(0, 199);    // medium
static std::uniform_int_distribution<int> spikeDist(0, 999);  // spike

int generateNetworkDelay() {
    int r = dist100(gen);

    if(r < 70) return fastDist(gen);       // 70% fast
    else if(r < 90) return medDist(gen);  // 20% medium
    else return spikeDist(gen);           // 10% spike
}

std::string getCurrentTime() {
    std::time_t now = std::time(nullptr);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));
    return std::string(buffer);
}

// Constructor
OrderBook::OrderBook(double minP, double maxP, double multiplier)
    : minPrice(minP), maxPrice(maxP), priceMultiplier(multiplier),
      bids(MAX_BUCKETS), asks(MAX_BUCKETS)
{
    logFile.open("latency_log.csv");
    if(!logFile.is_open()) std::cerr << "Error opening latency_log.csv\n";

    logFile << "order_id,network_us,queue_ns,engine_ns,total_us\n";
    logFile.tie(nullptr); // faster logging

    benchmarkStart = std::chrono::high_resolution_clock::now();
}

// Destructor
OrderBook::~OrderBook() {
    if(logFile.is_open()) logFile.close();
}

// ---------------- ADD LIMIT ORDER ----------------
void OrderBook::addLimitOrder(const Order& order) {
    auto startTotal = std::chrono::high_resolution_clock::now();
    ordersProcessed++;

    // ---------------- NETWORK DELAY SIM ----------------
    int networkDelay = generateNetworkDelay();
    std::this_thread::sleep_for(std::chrono::microseconds(networkDelay));

    Order newOrder = order;
    auto queueStart = std::chrono::high_resolution_clock::now();
    newOrder.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        queueStart.time_since_epoch()).count();

    // ---------------- INSERT INTO BOOK ----------------
    int idx = priceToIndex(newOrder.price);

    if(newOrder.side == Side::BUY) {
        bids[idx].push_back(newOrder);
        orderLookup[newOrder.id] = {idx, (int)bids[idx].size()-1};
    } else {
        asks[idx].push_back(newOrder);
        orderLookup[newOrder.id] = {idx, (int)asks[idx].size()-1};
    }

    auto queueEnd = std::chrono::high_resolution_clock::now();
    auto queuingDelay = std::chrono::duration_cast<std::chrono::nanoseconds>(queueEnd - queueStart).count();

    // ---------------- MATCHING ENGINE ----------------
    auto engineStart = std::chrono::high_resolution_clock::now();
    match();
    auto engineEnd = std::chrono::high_resolution_clock::now();
    auto engineLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(engineEnd - engineStart);

    // ---------------- TOTAL LATENCY ----------------
    auto endTotal = std::chrono::high_resolution_clock::now();
    auto totalLatency = std::chrono::duration_cast<std::chrono::microseconds>(endTotal - startTotal);

    // ---------------- CSV LOGGING ----------------
    logFile << newOrder.id << ","
            << networkDelay << ","
            << queuingDelay << ","
            << engineLatency.count() << ","
            << totalLatency.count() << "\n";
}

// ---------------- CANCEL ORDER ----------------
void OrderBook::cancelOrder(int id) {
    auto it = orderLookup.find(id);
    if(it == orderLookup.end()) return;

    int bucketIdx = it->second.first;
    int pos = it->second.second;

    std::deque<Order> &bucket = bids[bucketIdx].empty() ? asks[bucketIdx] : bids[bucketIdx];

    // Swap with back and pop for O(1)
    std::swap(bucket[pos], bucket.back());
    bucket.pop_back();

    // Update orderLookup for swapped order
    if(pos < bucket.size())
        orderLookup[bucket[pos].id] = {bucketIdx, pos};

    orderLookup.erase(it);
}

// ---------------- MATCH ----------------
void OrderBook::match() {
    for(int bidIdx = MAX_BUCKETS-1; bidIdx >= 0; --bidIdx) {
        if(bids[bidIdx].empty()) continue;
        for(int askIdx = 0; askIdx < MAX_BUCKETS; ++askIdx) {
            if(asks[askIdx].empty()) continue;

            auto &buyBucket = bids[bidIdx];
            auto &sellBucket = asks[askIdx];

            while(!buyBucket.empty() && !sellBucket.empty()) {
                Order &buyOrder = buyBucket.front();
                Order &sellOrder = sellBucket.front();
                int tradeQty = std::min(buyOrder.quantity, sellOrder.quantity);

                Trade trade;
                trade.buyOrderId = buyOrder.id;
                trade.sellOrderId = sellOrder.id;
                trade.price = sellOrder.price;
                trade.quantity = tradeQty;
                trade.timestamp = getCurrentTime();
                tradeHistory.push_back(trade);

                buyOrder.quantity -= tradeQty;
                sellOrder.quantity -= tradeQty;

                if(buyOrder.quantity == 0) buyBucket.pop_front();
                if(sellOrder.quantity == 0) sellBucket.pop_front();
            }
        }
    }
}

// ---------------- MARKET ORDER ----------------
void OrderBook::executeMarketOrder(Side side, int quantity) {
    if(side == Side::BUY) {
        for(int askIdx = 0; askIdx < MAX_BUCKETS && quantity > 0; ++askIdx) {
            auto &bucket = asks[askIdx];
            while(!bucket.empty() && quantity > 0) {
                Order &sellOrder = bucket.front();
                int tradeQty = std::min(quantity, sellOrder.quantity);
                sellOrder.quantity -= tradeQty;
                quantity -= tradeQty;
                if(sellOrder.quantity == 0) bucket.pop_front();
            }
        }
    } else {
        for(int bidIdx = MAX_BUCKETS-1; bidIdx >= 0 && quantity > 0; --bidIdx) {
            auto &bucket = bids[bidIdx];
            while(!bucket.empty() && quantity > 0) {
                Order &buyOrder = bucket.front();
                int tradeQty = std::min(quantity, buyOrder.quantity);
                buyOrder.quantity -= tradeQty;
                quantity -= tradeQty;
                if(buyOrder.quantity == 0) bucket.pop_front();
            }
        }
    }

    if(quantity > 0)
        std::cout << "Unfilled quantity: " << quantity << "\n";
}

// ---------------- PRINT ORDER BOOK ----------------
void OrderBook::printBook() const {
    std::cout << "----- ORDER BOOK -----\n";

    std::cout << "\nSELL ORDERS\n";
    for(int i = 0; i < MAX_BUCKETS; ++i)
        if(!asks[i].empty())
            std::cout << "Price: " << i / priceMultiplier + minPrice
                      << " Qty: " << asks[i].size() << "\n";

    std::cout << "\nBUY ORDERS\n";
    for(int i = MAX_BUCKETS-1; i >= 0; --i)
        if(!bids[i].empty())
            std::cout << "Price: " << i / priceMultiplier + minPrice
                      << " Qty: " << bids[i].size() << "\n";
}

// ---------------- PRINT TRADES ----------------
void OrderBook::printTrades() const {
    std::cout << "\n--- TRADE HISTORY ---\n";
    for(const auto &t : tradeHistory)
        std::cout << "BUY:" << t.buyOrderId
                  << " SELL:" << t.sellOrderId
                  << " PRICE:" << t.price
                  << " QTY:" << t.quantity
                  << " TIME:" << t.timestamp << "\n";
}

// ---------------- THROUGHPUT ----------------
void OrderBook::printThroughput() const {
    auto benchmarkEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(benchmarkEnd - benchmarkStart).count();
    long long throughput = ordersProcessed / std::max(1LL, duration);
    std::cout << "Orders Processed: " << ordersProcessed << "\n";
    std::cout << "Time Taken: " << duration << " sec\n";
    std::cout << "Throughput: " << throughput << " orders/sec\n";
}


































// #include "order_book.h"

// #include <iostream>
// #include <chrono>
// #include <thread>

// #include <random>
// #include <ctime>
// #include <fstream>

// std::ofstream logFile("latency_log.csv");
// std::string getCurrentTime() {

//     std::time_t now = std::time(nullptr);
//     char buffer[80];

//     std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));

//     return std::string(buffer);
// }

// static std::random_device rd;
// static std::mt19937 gen(rd());
// static std::uniform_int_distribution<> dist(20,100);
// void OrderBook::addLimitOrder(const Order& order) {

//     auto startTotal = std::chrono::high_resolution_clock::now();
//      ordersProcessed++;

//     // ---------------- NETWORK DELAY ----------------
//     int networkDelay = dist(gen);

//     std::this_thread::sleep_for(
//         std::chrono::microseconds(networkDelay)
//     );

//     // ---------------- COPY ORDER ----------------
//     Order newOrder = order;

//     // ---------------- QUEUE START ----------------
//     auto queueStart = std::chrono::high_resolution_clock::now();

//     // timestamp (AFTER network delay)
//     newOrder.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
//         queueStart.time_since_epoch()
//     ).count();

//     // ---------------- INSERT INTO BOOK ----------------
//     if(newOrder.side == Side::BUY) {

//         bids[newOrder.price].push_back(newOrder);
//         auto it = std::prev(bids[newOrder.price].end());
//         orderLookup[newOrder.id] = it;

//     } else {

//         asks[newOrder.price].push_back(newOrder);
//         auto it = std::prev(asks[newOrder.price].end());
//         orderLookup[newOrder.id] = it;
//     }

//     // ---------------- QUEUE END ----------------
//     auto queueEnd = std::chrono::high_resolution_clock::now();

//     auto queuingDelay =
//         std::chrono::duration_cast<std::chrono::nanoseconds>(queueEnd - queueStart).count();

//     // ---------------- MATCHING ENGINE ----------------
//     auto engineStart = std::chrono::high_resolution_clock::now();

//     match();

//     auto engineEnd = std::chrono::high_resolution_clock::now();

//     auto engineLatency =
//         std::chrono::duration_cast<std::chrono::nanoseconds>(engineEnd - engineStart);

//     // ---------------- TOTAL LATENCY ----------------
//     auto endTotal = std::chrono::high_resolution_clock::now();

//     auto totalLatency =
//         std::chrono::duration_cast<std::chrono::microseconds>(endTotal - startTotal);

//     // ---------------- PRINT (DEBUG) ----------------
//     std::cout << "Network Latency: " << networkDelay << " us\n";
//     std::cout << "Queuing Delay: " << queuingDelay << " ns\n";
//     std::cout << "Engine Latency: " << engineLatency.count() << " ns\n";
//     std::cout << "Total Latency: " << totalLatency.count() << " us\n";

//     // ---------------- CSV LOGGING ----------------
//     logFile << newOrder.id << ","
//             << networkDelay << ","
//             << queuingDelay << ","
//             << engineLatency.count() << ","
//             << totalLatency.count()
//             << "\n";
// }
// void OrderBook::printBook() {

//     std::cout << "----- ORDER BOOK -----\n";

//     std::cout << "\nSELL ORDERS\n";
//     for(auto &level : asks) {
//         std::cout << "Price: " << level.first
//                   << " Qty: " << level.second.size() << "\n";
//     }

//     std::cout << "\nBUY ORDERS\n";
//     for(auto &level : bids) {
//         std::cout << "Price: " << level.first
//                   << " Qty: " << level.second.size() << "\n";
//     }
// }

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

//         if(buyOrder.quantity == 0) {

//             orderLookup.erase(buyOrder.id);   // remove from lookup table
//             bestBid->second.pop_front();
//         }

//         if(sellOrder.quantity == 0) {

//             orderLookup.erase(sellOrder.id);  // remove from lookup table
//             bestAsk->second.pop_front();
//         }

//         if(bestBid->second.empty())
//             bids.erase(bestBid);

//         if(bestAsk->second.empty())
//             asks.erase(bestAsk);
//     }
// }

// void OrderBook::cancelOrder(int id) {
//     auto it = orderLookup.find(id);
//     if (it == orderLookup.end()) return;

//     auto orderIt = it->second;
//     auto price = orderIt->price;

//     if (orderIt->side == Side::BUY) {
//         auto &level = bids[price];
//         level.erase(orderIt);
//         if (level.empty()) bids.erase(price);
//     } else {
//         auto &level = asks[price];
//         level.erase(orderIt);
//         if (level.empty()) asks.erase(price);
//     }

//     orderLookup.erase(it);
// }
// void OrderBook::printTrades() {

//     std::cout << "\n--- TRADE HISTORY ---\n";

//     for(const auto &trade : tradeHistory) {

//         std::cout << "BUY:"
//                   << trade.buyOrderId
//                   << " SELL:"
//                   << trade.sellOrderId
//                   << " PRICE:"
//                   << trade.price
//                   << " QTY:"
//                   << trade.quantity
//                   << " TIME:"
//                   << trade.timestamp
//                   << "\n";
//     }
// }
// OrderBook::OrderBook() {
//     benchmarkStart = std::chrono::high_resolution_clock::now();
// }
// void OrderBook::executeMarketOrder(Side side, int quantity) {

//     if(side == Side::BUY) {

//         while(quantity > 0 && !asks.empty()) {

//             auto bestAsk = asks.begin();
//             double askPrice = bestAsk->first;

//             Order &sellOrder = bestAsk->second.front();

//             int tradeQty = std::min(quantity, sellOrder.quantity);

//             std::cout << "MARKET TRADE: "
//                       << tradeQty
//                       << " @ "
//                       << askPrice
//                       << "\n";

//             sellOrder.quantity -= tradeQty;
//             quantity -= tradeQty;

//             if(sellOrder.quantity == 0)
//                 bestAsk->second.pop_front();

//             if(bestAsk->second.empty())
//                 asks.erase(bestAsk);
//         }
//     }

//     else {

//         while(quantity > 0 && !bids.empty()) {

//             auto bestBid = bids.begin();
//             double bidPrice = bestBid->first;

//             Order &buyOrder = bestBid->second.front();

//             int tradeQty = std::min(quantity, buyOrder.quantity);

//             std::cout << "MARKET TRADE: "
//                       << tradeQty
//                       << " @ "
//                       << bidPrice
//                       << "\n";

//             buyOrder.quantity -= tradeQty;
//             quantity -= tradeQty;

//             if(buyOrder.quantity == 0)
//                 bestBid->second.pop_front();

//             if(bestBid->second.empty())
//                 bids.erase(bestBid);
//         }
//     }

//     if(quantity > 0)
//         std::cout << "Unfilled quantity: " << quantity << "\n";
// }
// void OrderBook::printThroughput() {

//     auto benchmarkEnd = std::chrono::high_resolution_clock::now();

//     auto duration = std::chrono::duration_cast<std::chrono::seconds>(
//         benchmarkEnd - benchmarkStart
//     ).count();

//     long long throughput = ordersProcessed / std::max(1LL, duration);

//     std::cout << "Orders Processed: " << ordersProcessed << "\n";
//     std::cout << "Time Taken: " << duration << " sec\n";
//     std::cout << "Throughput: " << throughput << " orders/sec\n";
// }
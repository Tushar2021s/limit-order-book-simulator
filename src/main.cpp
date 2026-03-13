#include "order_book.h"

int main() {

    OrderBook ob;

    Order o1{1, Side::BUY, 100, 10, OrderType::LIMIT};
    Order o2{2, Side::BUY, 101, 5, OrderType::LIMIT};
    Order o3{3, Side::SELL, 102, 7, OrderType::LIMIT};

    ob.addLimitOrder(o1);
    ob.addLimitOrder(o2);
    ob.addLimitOrder(o3);

    ob.printBook();

}
#include "order_book.h"
#include <iostream>
#include <string>

int main() {

    OrderBook ob;

    std::string command;

    while (true) {

        std::cout << "\nEnter Command (ADD / PRINT / EXIT): ";
        std::cin >> command;

        if (command == "ADD") {

            int id;
            std::string sideStr;
            double price;
            int quantity;

            std::cin >> id >> sideStr >> price >> quantity;

            Side side;

            if (sideStr == "BUY")
                side = Side::BUY;
            else
                side = Side::SELL;

            Order order{id, side, price, quantity, OrderType::LIMIT};

            ob.addLimitOrder(order);

        }

        else if (command == "PRINT") {

            ob.printBook();

        }

        else if (command == "EXIT") {

            break;

        }

        else {

            std::cout << "Invalid command\n";

        }
    }

    return 0;
}
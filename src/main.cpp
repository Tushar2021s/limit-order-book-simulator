#include "order_book.h"
#include <iostream>
#include <string>

int main() {

    OrderBook ob;

    std::string command;

    while (true) {

        std::cout << "\nEnter Command (ADD / PRINT / EXIT): ";
        std::cin >> command;

        // if (command == "ADD") {

        //     int id;
        //     std::string sideStr;
        //     double price;
        //     int quantity;

        //     std::cin >> id >> sideStr >> price >> quantity;

        //     Side side;

        //     if (sideStr == "BUY")
        //         side = Side::BUY;
        //     else
        //         side = Side::SELL;

        //     Order order{id, side, price, quantity, OrderType::LIMIT};

        //     ob.addLimitOrder(order);

        // }before adding the market orders excecution
        if(command == "ADD") {

            int id;
            std::string sideStr;
            std::string type;

            std::cin >> id >> sideStr >> type;

            Side side = (sideStr == "BUY") ? Side::BUY : Side::SELL;

            if(type == "LIMIT") {

                double price;
                int quantity;

                std::cin >> price >> quantity;

                Order order{id, side, price, quantity, OrderType::LIMIT};

                ob.addLimitOrder(order);
            }

            else if(type == "MARKET") {

                int quantity;
                std::cin >> quantity;

                ob.executeMarketOrder(side, quantity);
            }
        }

        else if (command == "PRINT") {

            ob.printBook();

        }

        else if (command == "EXIT") {

            break;

        }
        else if (command == "CANCEL") {

            int id;
            std::cin >> id;

            ob.cancelOrder(id);
        }
        else if(command == "TRADES") {
            ob.printTrades();
        }
        else {

            std::cout << "Invalid command\n";

        }
    }

    return 0;
}
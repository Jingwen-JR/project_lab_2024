#include <map>
#include <cstdint>
#include <utility>
#include <tuple>
#include "orderBook.h"
#include "bbo.h"

using namespace std;

const std::map<int64_t, uint32_t>& OrderBook::getOrders(uint8_t SideIndicator) const {
        return (SideIndicator == 'B') ? buyOrders : sellOrders;
    }

std::map<int64_t, uint32_t>& OrderBook::getOrders(uint8_t SideIndicator) {
        return (SideIndicator == 'B') ? buyOrders : sellOrders;
    }


// Add an order to the order book
std::pair<bool, BBO> OrderBook::add(int64_t price, uint32_t quantity, uint8_t sideIndicator) {
    // Get the best bid and ask before adding the order
    auto bboBefore = getBBO();

    // Add the order
    auto& orders = getOrders(sideIndicator);
    orders[price] += quantity;

    // Get the best bid and ask after adding the order
    auto bboAfter = getBBO();

    // Compare and update BBOs if there is any change
    if (bboBefore != bboAfter) {
        bbo.bidPrice = std::get<0>(bboAfter);
        bbo.bidQuantity = std::get<1>(bboAfter);
        bbo.askPrice = std::get<2>(bboAfter);
        bbo.askQuantity = std::get<3>(bboAfter);
        return { true, bbo };
    }
    return { false, bbo };
}


// Modify an order in the order book
std::pair<bool, BBO> OrderBook::modify(uint32_t quantity, int64_t price, uint32_t newQuantity, int64_t newPrice, uint8_t sideIndicator) {
    // Get the best bid and ask before modifying the orderbook
    auto bboBefore = getBBO();

    // Remove the old order
    auto& orders = getOrders(sideIndicator);
    auto it = orders.find(price);

    auto& ordersBuffer = (sideIndicator == 'B') ? buyOrdersBuffer : sellOrdersBuffer;
    auto bufferIt = ordersBuffer.find(price);

    uint32_t remainingQuantity = quantity;

    // 1.Remove the quantity from buffer orders first
    if (bufferIt != ordersBuffer.end()) {
        if (bufferIt->second > remainingQuantity) {
            bufferIt->second -= remainingQuantity;
            remainingQuantity = 0;
        }
        else {
            remainingQuantity -= bufferIt->second;
            ordersBuffer.erase(bufferIt);
        }
    }

    // 2. Remove the remaining quantity from main orders
    if (remainingQuantity > 0 && it != orders.end()) {
        if (it->second > remainingQuantity) {
            it->second -= remainingQuantity;
        }
        else if (it->second == remainingQuantity) {
            orders.erase(it);
        }
        else {
            cout << "Error in DEL/EXU: Quantity to DEL/EXU is greater than the available quantity" << endl;
        }
    }
    else if (remainingQuantity > 0) {
        cout << "Error in DEL/EXU: Quantity to DEL/EXU is greater than the available quantity" << endl;
    }

    // Add the new order
    orders[newPrice] += newQuantity;

    // Get the best bid and ask after modifying the orderbook
    auto bboAfter = getBBO();

    // Compare and update BBOs if there is any change
    if (bboBefore != bboAfter) {
        bbo.bidPrice = std::get<0>(bboAfter);
        bbo.bidQuantity = std::get<1>(bboAfter);
        bbo.askPrice = std::get<2>(bboAfter);
        bbo.askQuantity = std::get<3>(bboAfter);
        return { true, bbo };
    }
    return { false, bbo };
}



// Remove an order from the order book
std::pair<bool, BBO> OrderBook::remove(int64_t price, uint32_t quantity, uint8_t sideIndicator) {
    // Get the best bid and ask before removing the order
    auto bboBefore = getBBO();

    // Remove the order
    auto& orders = getOrders(sideIndicator);
    auto it = orders.find(price);

    auto& ordersBuffer = (sideIndicator == 'B') ? buyOrdersBuffer : sellOrdersBuffer;
    auto bufferIt = ordersBuffer.find(price);

    uint32_t remainingQuantity = quantity;

    // 1.Remove the quantity from buffer orders first
    if (bufferIt != ordersBuffer.end()) {
        if (bufferIt->second > remainingQuantity) {
            bufferIt->second -= remainingQuantity;
            remainingQuantity = 0;
        }
        else {
            remainingQuantity -= bufferIt->second;
            ordersBuffer.erase(bufferIt);
        }
    }

    // 2. Remove the remaining quantity from main orders
    if (remainingQuantity > 0 && it != orders.end()) {
        if (it->second > remainingQuantity) {
            it->second -= remainingQuantity;
        }
        else if (it->second == remainingQuantity) {
            orders.erase(it);
        }
        else {
            orders.erase(it);
            cout << "Error in DEL/EXU: Quantity to DEL/EXU is greater than the available quantity" << endl;
        }
    }
    else if (remainingQuantity > 0) {
        cout << "Error in DEL/EXU: Quantity to DEL/EXU is greater than the available quantity" << endl;
    }

    // Get the best bid and ask after removing the order
    auto bboAfter = getBBO();

    // Compare and update BBOs if there is any change
    if (bboBefore != bboAfter) {
        bbo.bidPrice = std::get<0>(bboAfter);
        bbo.bidQuantity = std::get<1>(bboAfter);
        bbo.askPrice = std::get<2>(bboAfter);
        bbo.askQuantity = std::get<3>(bboAfter);
        return { true, bbo };
    }
    return { false, bbo };
}


// Vitually execute an order:
// 1. Remove the order from the main order book
// 2. Add the order to the buffer order book
BBO OrderBook::vituallyExcecute(int64_t price, uint32_t quantity, uint8_t SideIndicator) {
    // Remove the order from the main order book
    auto& orders = getOrders(SideIndicator);
    auto it = orders.find(price);
    if (it != orders.end()) {
        if (it->second > quantity) {
            it->second -= quantity;
        }
        else if (it->second == quantity) {
            orders.erase(it);
        }
        else {
            cout << "Error in VEXC: Quantity to virtually execute is greater than the available quantity" << endl;
        }
    }
    else {
        cout << "Error in VEXC: No available quantity at the current price level" << endl;
    }

    // Add the order to the buffer order book
    auto& ordersBuffer = (SideIndicator == 'B') ? buyOrdersBuffer : sellOrdersBuffer;
    ordersBuffer[price] += quantity;

    // Get and return the updated Best Bid and Offer (BBO) after virtually executing the order
    auto bboAfter = getBBO();
    bbo.bidPrice = std::get<0>(bboAfter);
    bbo.bidQuantity = std::get<1>(bboAfter);
    bbo.askPrice = std::get<2>(bboAfter);
    bbo.askQuantity = std::get<3>(bboAfter);
    return bbo;
}


// Get the best bid and ask price and quantity
tuple<int64_t, uint32_t, int64_t, uint32_t> OrderBook::getBBO() const {
    auto bestBid = getBestBid();
    auto bestAsk = getBestAsk();
    return { bestBid.first, bestBid.second, bestAsk.first, bestAsk.second };
}


// Get the best bid price and quantity
std::pair<int64_t, uint32_t> OrderBook::getBestBid() const {
    if (buyOrders.empty()) {
        return { 0, 0 }; // No bids available
    }
    auto it = buyOrders.rbegin(); // Highest price in buy orders
    return { it->first, it->second };
}

// Get the best ask price and quantity
std::pair<int64_t, uint32_t> OrderBook::getBestAsk() const {
    if (sellOrders.empty()) {
        return { 0, 0 }; // No asks available
    }
    auto it = sellOrders.begin(); // Lowest price in sell orders
    return { it->first, it->second };
}


// Get the quantity at a given price level and side
uint32_t OrderBook::getQuantityAtPriceLevel(int64_t price, uint8_t sideIndicator) const {
    const auto& orders = getOrders(sideIndicator);
    auto it = orders.find(price);
    if (it != orders.end()) {
        return it->second;
    }
    return 0; // Price level not found
}


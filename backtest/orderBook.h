#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <cstdint>
#include <utility>
#include <tuple>
#include "bbo.h"

using namespace std;

class OrderBook {
public:
    const std::map<int64_t, uint32_t>& getOrders(uint8_t SideIndicator) const;

    std::map<int64_t, uint32_t>& getOrders(uint8_t SideIndicator);

    // Add an order to the order book
    std::pair<bool, BBO> add(int64_t price, uint32_t quantity, uint8_t sideIndicator);

    // Modify an order in the order book
    std::pair<bool, BBO> modify(uint32_t quantity, int64_t price, uint32_t newQuantity, int64_t newPrice, uint8_t sideIndicator);

    // Remove an order from the order book
    std::pair<bool, BBO> remove(int64_t price, uint32_t quantity, uint8_t sideIndicator);

    // Vitually execute an order for backtesting
    BBO vituallyExcecute(int64_t price, uint32_t quantity, uint8_t SideIndicator);

    // Get the best bid and ask price and quantity
    tuple<int64_t, uint32_t, int64_t, uint32_t> getBBO() const;

    // Get the best bid price and quantity
    std::pair<int64_t, uint32_t> getBestBid() const;

    // Get the best ask price and quantity
    std::pair<int64_t, uint32_t> getBestAsk() const;

    // Get the quantity at a given price level and side
    uint32_t getQuantityAtPriceLevel(int64_t price, uint8_t sideIndicator) const;

private:
    // Buy/Sell orders map (sorted low to high by default)
    std::map<int64_t, uint32_t> buyOrders;  
    std::map<int64_t, uint32_t> sellOrders;
    std::map<int64_t, uint32_t> buyOrdersBuffer;
    std::map<int64_t, uint32_t> sellOrdersBuffer;
    // Best bid and ask price and quantity
    BBO bbo;

};

#endif // ORDERBOOK_H

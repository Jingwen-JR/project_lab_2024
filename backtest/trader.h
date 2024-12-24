#pragma once
#include "observer.h"
#include "orderBook.h"
#include "orderManager.h"
#include "bbo.h"
#include "symbolMap.h"
#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <vector>
#include <chrono>

struct TradeLogEntry {
    std::string srcTime;
    std::string symbolName;
    double price;
    uint32_t quantity;
    char sideIndicator;

    TradeLogEntry(const std::string& srcTime, const std::string& symbolName, double price, uint32_t quantity, char sideIndicator)
        : srcTime(srcTime), symbolName(symbolName), price(price), quantity(quantity), sideIndicator(sideIndicator) {}
};



class Trader : public Observer {
public:
    // Constructor
    Trader(OrderManager& orderManager, const std::vector<std::pair<std::string, std::string>>& symbolPairs);

    // Destructor
    ~Trader();

    // Update method called by OrderManager anytime there is a change in the order book
    void update(const std::string& symbol, const std::string& srcTime) override;

    // Update the trade log whenever an arbitrage opportunity is traded
    void updateTradeLog(const std::string& srcTime, const std::string& symbol, double price, uint32_t quantity, char sideIndicator);

    // Perform arbitrage between two symbols
    void arbitrage(const std::string& symbolA, const std::string& symbolB);

    // Write the trade log to a CSV file
    void writeTradeLogToCsv(const std::string& filename) const;

private:
    OrderManager& orderManager; // Reference to OrderManager
    std::vector<std::pair<std::string, std::string>> symbolPairs; // Symbol pairs for arbitrage
    std::vector<TradeLogEntry> tradeLog; // Trade log entries
    const SymbolMap& symbolMap; // Reference to SymbolMap
    std::string srcTime; // Source time
    double pnl = 0; // Pnl for each arbitrage opportunity
    double totalPnl = 0; // Total cumulative Pnl
};

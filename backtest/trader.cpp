#include "trader.h"

// Calculate the minimum trading quantities for arbitrage
void calcMinTradingQty(int contractSizeA, int contractSizeB, int& minQtyA, int& minQtyB) {
    // Calculate the greatest common divisor (GCD) using the Euclidean algorithm
    int gcdValue = contractSizeA;
    int temp = contractSizeB;
    while (temp != 0) {
        int t = temp;
        temp = gcdValue % temp;
        gcdValue = t;
    }
    // Calculate the minimum trading quantities based on the GCD
    minQtyA = contractSizeB / gcdValue;
    minQtyB = contractSizeA / gcdValue;
}


// Display the order book (for debugging)
void displayOrderBook(const std::string& symbol, const OrderBook& orderBook) {
    std::cout << "Symbol: " << symbol << std::endl;
    std::cout << "Asks: " << std::endl;
    int count = 0;
    for (auto it = orderBook.getOrders('S').begin(); it != orderBook.getOrders('S').end() && count < 10; ++it, ++count) {
        int64_t price = it->first;
        uint32_t quantity = it->second;
        std::cout << "Price: " << price << ", Quantity: " << quantity << std::endl;
    }

    std::cout << "Bids: " << std::endl;
    count = 0;
    for (auto it = orderBook.getOrders('B').rbegin(); it != orderBook.getOrders('B').rend() && count < 10; ++it, ++count) {
        int64_t price = it->first;
        uint32_t quantity = it->second;
        std::cout << "Price: " << price << ", Quantity: " << quantity << std::endl;
    }
}


Trader::Trader(OrderManager& om, const std::vector<std::pair<std::string, std::string>>& symbolPairs)
    : orderManager(om), symbolPairs(symbolPairs), symbolMap(orderManager.symbolMap) {
    orderManager.registerObserver(this);
}

Trader::~Trader() {
    orderManager.unregisterObserver(this);
}


void Trader::update(const std::string& symbol, const std::string& srcTime) {
        for (const auto& pair : symbolPairs) {
            const auto& symbol1 = pair.first;
            const auto& symbol2 = pair.second;
            if ((symbol == symbol1 || symbol == symbol2)
                && orderManager.getTradingStatus(symbol1) == 'T'
                && orderManager.getTradingStatus(symbol2) == 'T') {
                this->srcTime = srcTime;
                arbitrage(symbol1, symbol2);
                arbitrage(symbol2, symbol1);
            }
        }
    }

void Trader::updateTradeLog(const std::string& srcTime, const std::string& symbol, double price, uint32_t quantity, char sideIndicator) {
    tradeLog.emplace_back(srcTime, symbol, price, quantity, sideIndicator);
}

    // Find arbitrage opportunities and calculate total PnL
void Trader::arbitrage(const std::string& symbolA, const std::string& symbolB) {
        /*
            This function arbitrages against symbolA's Ask orders and symbolB's Bid orders
            For better readability, the naming convention here is: A for "Ask" orders and B for "Bid" orders
        */

        // Step0: Get the symbol info and order books ready
        // Get the symbolIDs and contract sizes
        auto symbolInfoA = symbolMap.getSymbolInfoById(symbolA);
        auto symbolInfoB = symbolMap.getSymbolInfoById(symbolB);
        int sizeA = symbolInfoA.size;
        int sizeB = symbolInfoB.size;

        // Calculate the minimum trading quantities based on the contract sizes
        int minQtyA, minQtyB;
        calcMinTradingQty(sizeA, sizeB, minQtyA, minQtyB);

        // Get the order books
        auto& orderBookA = orderManager.getOrderBook(symbolA);
        auto& orderBookB = orderManager.getOrderBook(symbolB);

        const auto& sellOrders = orderBookA.getOrders('S');
        const auto& buyOrders = orderBookB.getOrders('B');

        //displayOrderBook(symbolA, orderBookA);
        //displayOrderBook(symbolB, orderBookB);
        

        // Step1: Calculate the total quantities to trade for arbitrage
        // Find arbitrage opportunities where bid price of symbolA > ask price of symbolB
        auto bidIt = buyOrders.rbegin();  // Highest price in buy orders
        auto askIt = sellOrders.begin();  // Lowest price in sell orders
        int64_t bidPrice = bidIt->first;
        uint32_t bidQuantity = bidIt->second;
        int64_t askPrice = askIt->first;
        uint32_t askQuantity = askIt->second;

        uint32_t tradeQty = 0;
        uint32_t tradeQtyA = 0;
        uint32_t tradeQtyB = 0;

        uint32_t totalTradedAskQuantity = 0;
        uint32_t totalTradedBidQuantity = 0;

        while (bidIt != buyOrders.rend() && askIt != sellOrders.end() && bidPrice > askPrice) {

            tradeQty = std::min(bidQuantity / minQtyB, askQuantity / minQtyA);
            tradeQtyA = tradeQty * minQtyA;
            tradeQtyB = tradeQty * minQtyB;

            askQuantity -= tradeQtyA;
            bidQuantity -= tradeQtyB;

            totalTradedAskQuantity += tradeQtyA;
            totalTradedBidQuantity += tradeQtyB;

             // Move to the next ask if the current ask is fully traded or less than minimum trading quantity
            if (askQuantity < minQtyA) {
                ++askIt;
                askPrice = askIt->first;
                askQuantity += askIt->second;
            }

            // Move to the next bid if the current bid is fully traded or less than minimum trading quantity
            if (bidQuantity < minQtyB) {
                ++bidIt;
                bidPrice = bidIt->first;
                bidQuantity += bidIt->second;
            }
        }


        if (totalTradedAskQuantity == 0) {
            return;
        }
        //cout << srcTime << endl;
        //cout << totalTradedAskQuantity << " " << totalTradedBidQuantity << endl;

        // Step2: Calculate total PnL
        uint32_t countTotalTradedBidQuantity = 0;
        uint32_t countTotalTradedAskQuantity = 0;

        bidIt = buyOrders.rbegin();  // Highest price in buy orders
        askIt = sellOrders.begin();  // Lowest price in sell orders

        // if (srcTime == "2024-10-06 21:48:55.045412000") {
        //     displayOrderBook(symbolA, orderBookA);
        //     displayOrderBook(symbolB, orderBookB);}

		while (bidIt != buyOrders.rend() && countTotalTradedBidQuantity < totalTradedBidQuantity) {
            uint32_t quantity = std::min(bidIt->second, totalTradedBidQuantity - countTotalTradedBidQuantity);
            pnl += static_cast<double> (bidIt->first) / 100 * quantity * sizeB; // positive cashflows for selling the bid
            countTotalTradedBidQuantity += quantity;
			cout << srcTime << " Sell " << symbolB << " at " << static_cast<double> (bidIt->first) / 100 << ", quantity: " << quantity << ", contract size: " << sizeB << '\n';
            updateTradeLog(srcTime, symbolMap.getSymbolInfoById(symbolB).symbolName, static_cast<double> (bidIt->first) / 100, quantity,  'S');
			orderManager.vituallyExecuteOrder(srcTime, symbolB, bidIt->first, quantity, 'B');
            bidIt = buyOrders.rbegin();
        }
		
        while (askIt != sellOrders.end() && countTotalTradedAskQuantity < totalTradedAskQuantity) {
            uint32_t quantity = std::min(askIt->second, totalTradedAskQuantity - countTotalTradedAskQuantity);
            pnl -= static_cast<double> (askIt->first) / 100 * quantity * sizeA; // negative cashflows for buying the offer
            countTotalTradedAskQuantity += quantity;
			cout << srcTime << " Buy " << symbolA << " at " << static_cast<double> (askIt->first) / 100 << ", quantity: " << quantity << ", contract size: " << sizeA << '\n';
            updateTradeLog(srcTime, symbolMap.getSymbolInfoById(symbolA).symbolName, static_cast<double> (askIt->first) / 100, quantity, 'B');
            orderManager.vituallyExecuteOrder(srcTime, symbolA, askIt->first, quantity, 'S');
            askIt = sellOrders.begin();
        }

        if (pnl < 0) {
            std::cout << "Negative PnL! " << std::endl;
        }

        totalPnl += pnl;
        std::cout << "PnL: " << pnl << std::endl;
        std::cout << "Cumulative PnL: " << totalPnl << '\n' << endl;
        //displayOrderBook(symbolA, orderBookA);
        //displayOrderBook(symbolB, orderBookB);
        pnl = 0;

    }


void Trader::writeTradeLogToCsv(const std::string& filename) const {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << "Time,Symbol,Price,Quantity,SideIndicator\n";
            for (const auto& entry : tradeLog) {
                file << entry.srcTime << ","
                    << entry.symbolName << ","
                    << entry.price << ","
                    << entry.quantity << ","
                    << entry.sideIndicator << "\n";
            }
            file.close();
            std::cout << "Trade log written to file: " << filename << std::endl;
        }
        else {
            std::cerr << "Unable to open file: " << filename << std::endl;
        }
}
#include "order.h"
#include "orderBook.h"
#include "bbo.h"
#include "observer.h"
#include "symbolMap.h"
#include "orderManager.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;



void OrderManager::registerObserver(Observer* observer) {
        observers.push_back(observer);
    }

void OrderManager::unregisterObserver(Observer* observer) {
    observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
}


void OrderManager::notifyObservers(const string& symbol, const char* srcTime) {
    for (Observer* observer : observers) {
        observer->update(symbol, string(srcTime, 29).c_str());
    }
}

void OrderManager::updateCurrentDate(const string& Date) {
    currentDate = Date;
}

void OrderManager::updateCurrentTradeDate(const string& Date) {
    currentTradeDate = Date;
}

bool OrderManager::updateCurrentTime(const char* Time) {
    if (std::strncmp(Time, currentTime, 29) != 0) {
        std::copy(Time, Time + 29, std::begin(currentTime));
        return true;
    }
    return false;
}


void OrderManager::updateTradingStatus(const string& symbol, char status) {
    tradingStatus[symbol] = status;
}

const char* OrderManager::getCurrentTime() const {
    return currentTime;
}

char OrderManager::getTradingStatus(const string& symbol) const {
    auto it = tradingStatus.find(symbol);
    return (it != tradingStatus.end()) ? it->second : 'Q';
}

OrderBook& OrderManager::getOrderBook(const string& symbol) {
    return orderBooks.at(symbol);
}


void OrderManager::addOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, Order& order) {
    const char* oldTime = currentTime;
    string ssymbol = string(order.Symbol, 6);

    // Notify observers if the time was updated
    if (updateCurrentTime(srcTime)) {
        notifyObservers(ssymbol, oldTime);
    }
    
    // Update the order book and BBO
    auto result = orderBooks[ssymbol].add(order.Price, order.Quantity, order.SideIndicator);
    if (result.first) {
        const BBO& updatedBBO = result.second;
        bboHistory.emplace_back(BBORecord{ srcTime, pktSeqNum, msgSeqNum, 'A', order.Symbol, updatedBBO.bidPrice, updatedBBO.bidQuantity, updatedBBO.askPrice, updatedBBO.askQuantity, getTradingStatus(ssymbol) });
    }

    // Update the order store
    orderStore[order.OrderId] = order;

}

void OrderManager::modifyOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId, uint32_t newQuantity, int64_t newPrice) {
    const char* oldTime = currentTime;
    auto it = orderStore.find(OrderId);
    if (it != orderStore.end()) {
        // Notify observers if the time was updated
        string ssymbol = string(it->second.Symbol, 6);
        if (updateCurrentTime(srcTime)) {
            notifyObservers(ssymbol, oldTime);
        }

        // Update the order book and BBO
        auto result = orderBooks.at(ssymbol).modify(it->second.Quantity, it->second.Price, newQuantity, newPrice, it->second.SideIndicator);
        if (result.first) {
            const BBO& updatedBBO = result.second;
            bboHistory.emplace_back(BBORecord{ srcTime, pktSeqNum, msgSeqNum, 'M', it->second.Symbol, updatedBBO.bidPrice, updatedBBO.bidQuantity, updatedBBO.askPrice, updatedBBO.askQuantity, getTradingStatus(ssymbol) });
        }
        //Update the order store
        it->second.Quantity = newQuantity;
        it->second.Price = newPrice;

    }
    else {
        cerr << "Error in modifyOrder: OrderId " << OrderId << " not found in Order Store. Unable to modify order." << endl;
    }

}

void OrderManager::reduceSize(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId, uint32_t CancelledQuantity) {
    const char* oldTime = currentTime;
    auto it = orderStore.find(OrderId);
    if (it != orderStore.end()) {
        if (it->second.Quantity < CancelledQuantity) {
            cerr << "Error in reduceSize: Cannot reduce order size below 0 for OrderId " << OrderId << "." << endl;
            return;
        }

        // Notify observers
        string ssymbol = string(it->second.Symbol, 6);
        if (updateCurrentTime(srcTime)) {
            notifyObservers(ssymbol, oldTime);
        }

        // Update the order book and BBO
        auto result = orderBooks.at(ssymbol).remove(it->second.Price, CancelledQuantity, it->second.SideIndicator);
        if (result.first) {
            const BBO& updatedBBO = result.second;
            bboHistory.emplace_back(BBORecord{ srcTime, pktSeqNum, msgSeqNum, 'R', it->second.Symbol, updatedBBO.bidPrice, updatedBBO.bidQuantity, updatedBBO.askPrice, updatedBBO.askQuantity, getTradingStatus(ssymbol) });
        }
        //Update the order store
        it->second.Quantity -= CancelledQuantity;

    }
    else {
        cerr << "Error in reduceSize: OrderId " << OrderId << " not found in Order Store." << endl;
    }
}


void OrderManager::deleteOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId) {
    const char* oldTime = currentTime;
    auto it = orderStore.find(OrderId);
    if (it != orderStore.end()) {
        // Notify observers
        string ssymbol = string(it->second.Symbol, 6);
        if (updateCurrentTime(srcTime)) {
            notifyObservers(ssymbol, oldTime);
        }

        // Update the order book and BBO
        auto result = orderBooks.at(ssymbol).remove(it->second.Price, it->second.Quantity, it->second.SideIndicator);
        if (result.first) {
            const BBO& updatedBBO = result.second;
            bboHistory.emplace_back(BBORecord{ srcTime, pktSeqNum, msgSeqNum, 'D', it->second.Symbol, updatedBBO.bidPrice, updatedBBO.bidQuantity, updatedBBO.askPrice, updatedBBO.askQuantity, getTradingStatus(ssymbol) });
        }

        // Update the order store
        orderStore.erase(it);

    }
    else {
        cerr << "Error in deleteOrder: OrderId " << OrderId << " not found in Order Store." << endl;
    }
}

void OrderManager::executeOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId, uint32_t ExecutedQuantity) {
    const char* oldTime = currentTime;
    auto it = orderStore.find(OrderId);
    if (it != orderStore.end()) {
        if (ExecutedQuantity > it->second.Quantity) {
            cerr << "Error in executeOrder: Executed quantity " << ExecutedQuantity << " exceeds order quantity " << it->second.Quantity << " for OrderId " << OrderId << "." << endl;
            return;
        }

        // Notify observers
        string ssymbol = string(it->second.Symbol, 6);
        if (updateCurrentTime(srcTime)) {
            notifyObservers(ssymbol, oldTime);
        }

        // Update the order book and BBO
        auto result = orderBooks.at(ssymbol).remove(it->second.Price, ExecutedQuantity, it->second.SideIndicator);
        if (result.first) {
            const BBO& updatedBBO = result.second;
            bboHistory.emplace_back(BBORecord{ srcTime, pktSeqNum, msgSeqNum, 'E', it->second.Symbol, updatedBBO.bidPrice, updatedBBO.bidQuantity, updatedBBO.askPrice, updatedBBO.askQuantity, 'T' });
        }

        //Update the order store
        if (it->second.Quantity <= ExecutedQuantity) {
            orderStore.erase(it);
        }
        else {
            it->second.Quantity -= ExecutedQuantity;
        }

        //Update the trade volume
        updateTradeVolume(ssymbol, ExecutedQuantity, currentTradeDate);

    }
    else {
        cerr << "Error in executeOrder: OrderId " << OrderId << " not found in Order Store." << endl;
    }
}

void OrderManager::vituallyExecuteOrder(string ssrcTime, string ssymbol, int64_t price, uint32_t quantity, uint8_t sideIndicator) {
    // Convert 
    char symbol[6] = { 0 };
    std::copy(ssymbol.begin(), ssymbol.end(), symbol);

    char srcTime[29] = { 0 };
    std::copy(ssrcTime.begin(), ssrcTime.end(), srcTime);

    // Update the order book and BBO
    const BBO& updatedBBO = orderBooks.at(ssymbol).vituallyExcecute(price, quantity, sideIndicator);
    bboHistory.emplace_back(BBORecord{ srcTime, 0,0, 'E', symbol, updatedBBO.bidPrice, updatedBBO.bidQuantity, updatedBBO.askPrice, updatedBBO.askQuantity, 'T'});

}



void OrderManager::writeOrderBookToCsv(const string& filename) const {
    ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    csvFile << "Symbol,Price,Quantity,Side\n";
    for (const auto& symbolEntry : orderBooks) {
        const OrderBook& orderBook = symbolEntry.second;
        const string& symbol = symbolMap.getSymbolInfoById(symbolEntry.first).symbolName;

        // Write sell orders (high to low)
        for (auto it = orderBook.getOrders('S').rbegin(); it != orderBook.getOrders('S').rend(); ++it) {
            csvFile << symbol << ","
                << static_cast<double>(it->first) * 10e-3 << ","
                << it->second << ",Sell\n";
        }

        // Write buy orders (high to low)
        for (auto it = orderBook.getOrders('B').rbegin(); it != orderBook.getOrders('B').rend(); ++it) {
            csvFile << symbol << ","
                << static_cast<double>(it->first) * 10e-3 << ","
                << it->second << ",Buy\n";
        }
    }

    csvFile.close();
    cout << "Order Book written to " << filename << endl;
}


void OrderManager::writeOrderStoreToCsv(const string& filename) const {
    ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    csvFile << "OrderId,Quantity,Price,Symbol,Side\n";


    for (const auto& orderPair : orderStore) {
        const Order& order = orderPair.second;
        csvFile << order.OrderId << ","
            << order.Quantity << ","
            << static_cast<double>(order.Price) * 10e-3 << ","
            << string(order.Symbol, 6) << ","
            << order.SideIndicator << "\n";
    }

    csvFile.close();
    cout << "Order Store written to " << filename << endl;
}

void OrderManager::writeTradeVolumeToCsv(const string& filename) const {
    ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    csvFile << "Date,Symbol,ExecutedQuantity\n";
    for (const auto& dateEntry : tradeVolume) {
        const string& date = dateEntry.first;
        for (const auto& symbolEntry : dateEntry.second) {
            csvFile << date << ","
                << symbolMap.getSymbolInfoById(symbolEntry.first).symbolName << ","
                << symbolEntry.second << "\n";
        }
    }

    csvFile.close();
    cout << "Trade Volume written to " << filename << endl;
}

void OrderManager::writeBBOToCsv(const std::string& filename) const {
    std::ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    csvFile << "Time,PktSeqNum,MsgSeqNum,MsgType,Symbol,BidPrice,BidQuantity,AskPrice,AskQuantity,TradingStatus\n";
    for (const auto& record : bboHistory) {

        csvFile << string(record.srcTime, 29) << ","
            << record.pktSeqNum << ","
            << record.msgSeqNum << ","
            << record.msgType << ","
            << symbolMap.getSymbolInfoById(string(record.symbol,6)).symbolName << ","
            << static_cast<double>(record.bidPrice) * 10e-3 << ","
            << record.bidQuantity << ","
            << static_cast<double>(record.askPrice) * 10e-3 << ","
            << record.askQuantity << ","
            << record.tradingStatus << "\n";
    }

    csvFile.close();
    if (csvFile.fail()) {
        std::cerr << "Error writing to file: " << filename << std::endl;
    }
    else {
        std::cout << "BBO data written to " << filename << std::endl;
    }
}

void OrderManager::updateTradeVolume(string symbol, uint32_t executedQuantity, const string& date) {
    tradeVolume[date][symbol] += executedQuantity;
}


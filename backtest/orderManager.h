#pragma once
#include "order.h"
#include "orderBook.h"
#include "bbo.h"
#include "observer.h"
#include "symbolMap.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;


// Order Manager class
class OrderManager {
public:
    OrderManager() : currentTime{ 0 } {} // Initialize currentTime


    void registerObserver(Observer* observer);

    void unregisterObserver(Observer* observer);

    void notifyObservers(const string& symbol, const char* srcTime) ;

    void updateCurrentDate(const string& Date);

    void updateCurrentTradeDate(const string& Date);

    bool updateCurrentTime(const char* Time);

    void updateTradingStatus(const string& symbol, char status);

    const char* getCurrentTime() const;

    char getTradingStatus(const string& symbol) const;

    OrderBook& getOrderBook(const string& symbol);

    void addOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, Order& order);

    void modifyOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId, uint32_t newQuantity, int64_t newPrice);

    void reduceSize(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId, uint32_t CancelledQuantity);

    void deleteOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId);

    void executeOrder(const char srcTime[29], uint64_t pktSeqNum, uint64_t msgSeqNum, uint64_t OrderId, uint32_t ExecutedQuantity);

    void vituallyExecuteOrder(string ssrcTime, string ssymbol, int64_t price, uint32_t quantity, uint8_t sideIndicator);

    void writeOrderBookToCsv(const string& filename) const;

    void writeOrderStoreToCsv(const string& filename) const;

    void writeTradeVolumeToCsv(const string& filename) const;

    void writeBBOToCsv(const std::string& filename) const;

    void updateTradeVolume(string symbol, uint32_t executedQuantity, const string& date);

    SymbolMap symbolMap;


private:
    string currentTradeDate;
    string currentDate;
	char currentTime[29];
	unordered_map<uint64_t, Order> orderStore;  // OrderId -> Order
    unordered_map<string, OrderBook> orderBooks; // Symbol -> Order Book
    unordered_map<string, unordered_map<string, uint64_t>> tradeVolume; // Date -> Symbol -> Executed Quantity
	unordered_map<string, char> tradingStatus; // Symbol -> Trading Status
    vector<BBORecord> bboHistory;
    vector<Observer*> observers;

};


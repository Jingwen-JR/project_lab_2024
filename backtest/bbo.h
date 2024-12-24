#ifndef BBO_H
#define BBO_H

#pragma once
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <array>

using namespace std;


#pragma pack(push, 1)
struct BBO {
    int64_t bidPrice;              // Bid price
    uint32_t bidQuantity;          // Bid quantity
    int64_t askPrice;              // Ask price
    uint32_t askQuantity;          // Ask quantity
};
static_assert(sizeof(BBO) == 24, "BBO struct must be 24 bytes");


struct BBORecord {
    char srcTime[29];              // Exchange timestamp
    uint64_t pktSeqNum;            // Packet sequence number
    uint64_t msgSeqNum;            // Message sequence number
    char msgType;                  // Message type ("A" for Add; "D" for Delete; "M" for Modify; "E" for Execute; "R" for Reduce)
    char symbol[6];                // Symbol
    int64_t bidPrice;              // Bid price (converted to a floating-point value by scaling it down by a factor of 100)
    uint32_t bidQuantity;          // Bid quantity
    int64_t askPrice;              // Ask price (converted to a floating-point value by scaling it down by a factor of 100)
    uint32_t askQuantity;          // Ask quantity
    char tradingStatus;            // Current trading status
    
    // Default constructor
    BBORecord() = default;

    // Constructor for easy initialization
    BBORecord(
        const char* srcTime_, 
        uint64_t pktSeqNum_, 
        uint64_t msgSeqNum_, 
        char msgType_,
        const char* symbol_, 
        int64_t bidPrice_, 
        uint32_t bidQuantity_,
        int64_t askPrice_, 
        uint32_t askQuantity_, 
        char tradingStatus_
    )
        : 
        pktSeqNum(pktSeqNum_), 
        msgSeqNum(msgSeqNum_), 
        msgType(msgType_),
        bidPrice(bidPrice_), 
        bidQuantity(bidQuantity_),
        askPrice(askPrice_), 
        askQuantity(askQuantity_),
        tradingStatus(tradingStatus_)
    {
        std::memcpy(srcTime, srcTime_, sizeof(srcTime));
        std::memcpy(symbol, symbol_, sizeof(symbol));
    }
};

static_assert(sizeof(BBORecord) == 77, "BBORecord struct must be 77 bytes");

#pragma pack(pop)

#endif // BBO_H

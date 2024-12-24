#pragma once
#include <cstdint>
#include <cstring>
#include <array>
#include <string>

// Order structure
#pragma pack(push, 1)

struct Order {
    uint64_t OrderId;
    uint8_t SideIndicator;
    uint32_t Quantity;
    char Symbol[6];
    int64_t Price;

    // Default constructor
    Order() = default;
    
	// Constructor for easy initialization
    Order(const uint64_t OrderId_, uint8_t SideIndicator_, uint32_t Quantity_, const char* Symbol_, int64_t Price_)
        : OrderId(OrderId_), SideIndicator(SideIndicator_), Quantity(Quantity_), Price(Price_) {
        std::memcpy(Symbol, Symbol_, sizeof(Symbol));
    }
};

static_assert(sizeof(Order) == 27, "Order struct must be 27 bytes");

#pragma pack(pop)
#pragma once
#include <string>


class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(const std::string& symbol, const std::string& srcTime) = 0;
};

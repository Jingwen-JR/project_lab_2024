cmake_minimum_required(VERSION 3.10)

project(Backtest VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Set optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -o2")

# Define the executable target
add_executable(Backtest Backtest.cpp trader.cpp orderBook.cpp orderManager.cpp)

# Link the pcap library to the Backtest target
target_link_libraries(Backtest pcap)

# Set the output directory for the executable
set_target_properties(Backtest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
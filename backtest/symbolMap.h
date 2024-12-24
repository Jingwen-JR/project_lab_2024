// SymbolMap.h
#pragma once
#include "cfepitch.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>

using namespace std;

struct SymbolInfo {
    string symbolName;
    string symbolId;
    string legs;
    double tick;
    uint16_t size;
    string product;
    string root;
    string expiry;
    string listingState;
};

class SymbolMap {
public:
    void updateSymbolMap(const FuturesInstrumentDefinition& m, const unsigned char* message) {
        // Extract the symbolId
        string symbolId(reinterpret_cast<const char*>(m.Symbol), 6);

        // Extract the symbolName, product, legs
        string reportSymbol;
        string symbolName;
        string product;
        string legs;

        for (uint8_t i : m.ReportSymbol) {
            char c = static_cast<char>(i);
            if (isalnum(c)) {
                reportSymbol += c;
            }
            else {
                break;
            }
        }

        if (m.LegCount == 0) {
            int year_code = (m.ExpirationDate / 10000) % 10;
            int month = (m.ExpirationDate / 100) % 100;
            const char* monthCodes = "FGHJKMNQUVXZ";
            char month_code = (month >= 1 && month <= 12) ? monthCodes[month - 1] : '\0';
            if (month_code == '\0') {
                cerr << "month_code not recognizable error" << endl;
                return;
            }
            symbolName = reportSymbol + month_code + to_string(year_code);
            product = reportSymbol + "/" + month_code + to_string(year_code);
			legs = "na";
        }
        else {
            uint8_t* baseAddress = (uint8_t*)message + m.LegOffset - 2;
            for (int i = 0; i < m.LegCount; ++i) {
                int32_t leg_ratio = *reinterpret_cast<int32_t*>(baseAddress + 10 * i);
                string leg_symbol(reinterpret_cast<char*>(baseAddress + 4 + 10 * i), 6);
                product += getSymbolInfoById(leg_symbol).symbolName;
                if (leg_ratio == 1) {
                    symbolName += "+";
                    legs += "+" + leg_symbol;
                    product += ":1:B";
                }
                else if (leg_ratio == -1) {
                    symbolName += "-";
                    legs += "-" + leg_symbol;
                    product += ":1:S - ";
                }
                else {
                    cerr << "an error occur (leg ratio error)" << endl;
                    return;
                }
                symbolName += getSymbolInfoById(leg_symbol).symbolName;
            }
        }

        // Extract the tick size
        double tick = static_cast<double>(m.PriceIncrement) / 10000;

        // Extract the expiry date
        string date_str = to_string(m.ExpirationDate);
        string expiry = date_str.substr(0, 4) + "-" + date_str.substr(4, 2) + "-" + date_str.substr(6, 2);

        // Extract the symbol root
        string root;
        for (size_t i = 0; i < reportSymbol.length(); ++i) {
            if (!isalpha(reportSymbol[i])) {
                root = reportSymbol.substr(0, i);
                break;
            }
            else if (i == reportSymbol.length() - 1) {
                root = reportSymbol;
            }
        }

        // Extract the listing state
        string listingState;
        switch (static_cast<char>(m.ListingState)) {
        case 'A':
            listingState = "Active";
            break;
        case 'I':
            listingState = "Inactive";
            break;
        case 'T':
            listingState = "Test";
            break;
        default:
            listingState = "Unknown";
            break;
        }

        // Create a new SymbolInfo object
        SymbolInfo symbolInfo{ symbolName, symbolId, legs, tick, m.ContractSize, product, root, expiry, listingState };

        // Update the symbol map
        symbolMap[symbolId] = std::move(symbolInfo);
    }

    const SymbolInfo& getSymbolInfoByName(const std::string& symbolName) const {
        auto it = std::find_if(symbolMap.begin(), symbolMap.end(), [&symbolName](const auto& pair) {
            return pair.second.symbolName == symbolName;
            });
        if (it != symbolMap.end()) {
            return it->second;
        }
        else {
            throw std::runtime_error("Symbol not found in symbol map");
        }
    }

    const SymbolInfo& getSymbolInfoById(const std::string& symbolId) const {
        auto it = symbolMap.find(symbolId);
        if (it != symbolMap.end()) {
            return it->second;
        }
        else {
            throw std::runtime_error("Symbol not found in symbol map");
        }
    }

    void writeSymbolMapToCsv(const std::string& filename) const {
        std::ofstream file(filename);
        if (file.is_open()) {
            // Write the header line
            file << "symbolName,symbolId,legs,tick,size,product,root,expiry,listingState\n";
            for (const auto& pair : symbolMap) {
                const auto& symbolInfo = pair.second;
                file << symbolInfo.symbolName << ","
                    << symbolInfo.symbolId << ","
                    << symbolInfo.legs << ","
                    << symbolInfo.tick << ","
                    << symbolInfo.size << ","
                    << symbolInfo.product << ","
                    << symbolInfo.root << ","
                    << symbolInfo.expiry << ","
                    << symbolInfo.listingState << "\n";
            }
            file.close();
            std::cout << "Symbol map written to file: " << filename << std::endl;
        }
        else {
            std::cerr << "Unable to open file for writing." << std::endl;
        }
    }

    void loadSymbolMapFromCsv(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;

            // Skip the header line
            std::getline(file, line);

            while (std::getline(file, line)) {
                std::istringstream ss(line);
                std::string token;

                SymbolInfo symbolInfo;

                std::getline(ss, symbolInfo.symbolName, ',');
                std::getline(ss, symbolInfo.symbolId, ',');
                std::getline(ss, symbolInfo.legs, ',');
                std::getline(ss, token, ',');
                symbolInfo.tick = std::stod(token);
                std::getline(ss, token, ',');
                symbolInfo.size = static_cast<uint16_t>(std::stoi(token));
                std::getline(ss, symbolInfo.product, ',');
                std::getline(ss, symbolInfo.root, ',');
                std::getline(ss, symbolInfo.expiry, ',');
                std::getline(ss, symbolInfo.listingState, ',');

                symbolMap[symbolInfo.symbolId] = symbolInfo;
            }

            file.close();
            std::cout << "Symbol map read from file: " << filename << std::endl;
        }
        else {
            std::cerr << "Unable to open file for reading." << std::endl;
        }
    }


    std::vector<std::pair<std::string, std::string>> getVX_VXM_symbolPairs() {
        std::vector<std::pair<std::string, std::string>> symbolNamePairList;
        for (const auto& entry : symbolMap) {
            const auto& symbolData = entry.second;
            if (symbolData.legs == "na" && symbolData.listingState == "Active" && symbolData.root == "VXM") {
                std::string VXMsymbolID = entry.first;
                std::string VXsymbolName = "VX" + symbolData.symbolName.substr(3);
                std::string VXsymbolID = getSymbolInfoByName(VXsymbolName).symbolId;
                symbolNamePairList.emplace_back(VXMsymbolID, VXsymbolID);
				cout << "VXM symbol: " << symbolData.symbolName << " VX symbol: " << VXsymbolName << endl;
            }
        }
        return symbolNamePairList;
    }

private:
    std::unordered_map<std::string, SymbolInfo> symbolMap;
};

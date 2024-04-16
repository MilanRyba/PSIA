#include "CRC.h"
#include <iostream>
#include <string>

// Function to calculate CRC-32 for a given input data
uint32_t calculateCRC32(const char* data, size_t size) {
    return CRC::Calculate(data, size, CRC::CRC_32());
}

int main() {
    // Example input data
    std::string inputData = "Hello, world!"; // Original input data

    // Calculate CRC-32 for the original input data
    uint32_t senderCRC = calculateCRC32(inputData.c_str(), inputData.size());

    // Print the CRC-32 calculated by the sender for the original input data
    std::cout << "CRC-32 calculated by the sender (original data): " << std::hex << senderCRC << std::endl;

    // Simulate receiving the data
    // In a real scenario, you would receive the data over the network

    // Modify the input data (change 'H' to 'h')
    std::string modifiedData = inputData;
    modifiedData[0] = 'H';

    // Calculate CRC-32 for the modified input data
    uint32_t receiverCRC = calculateCRC32(modifiedData.c_str(), modifiedData.size());

    // Print the CRC-32 calculated by the receiver for the modified input data
    std::cout << "CRC-32 calculated by the receiver (modified data): " << std::hex << receiverCRC << std::endl;

    // Compare CRC-32 values
    if (senderCRC == receiverCRC) {
        std::cout << "CRC-32 values match. Data integrity preserved." << std::endl;
    }
    else {
        std::cout << "CRC-32 values do not match. Data integrity compromised." << std::endl;
    }

    return 0;
}

#include "CRC.h"
#include <iostream>
#include <string>

// Function to calculate CRC-32 for a given input data
uint32_t calculateCRC32(const char* data, size_t size) {
    return CRC::Calculate(data, size, CRC::CRC_32());
}

int main() {
    // Example input data
    std::string inputData = "Hello, world!";

    // Calculate CRC-32 for the input data
    uint32_t senderCRC = calculateCRC32(inputData.c_str(), inputData.size());

    // Print the CRC-32 calculated by the sender
    std::cout << "CRC-32 calculated by the sender: " << std::hex << senderCRC << std::endl;

    // Simulate receiving the data
    // In a real scenario, you would receive the data over the network

    // Assume the received data is the same as the input data
    std::string receivedData = inputData;

    // Calculate CRC-32 for the received data
    uint32_t receiverCRC = calculateCRC32(receivedData.c_str(), receivedData.size());

    // Print the CRC-32 calculated by the receiver
    std::cout << "CRC-32 calculated by the receiver: " << std::hex << receiverCRC << std::endl;

    // Compare CRC-32 values
    if (senderCRC == receiverCRC) {
        std::cout << "CRC-32 values match. Data integrity preserved." << std::endl;
    }
    else {
        std::cout << "CRC-32 values do not match. Data integrity compromised." << std::endl;
    }

    return 0;
}

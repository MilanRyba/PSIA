// #include "stdafx.h"
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include <thread>

// #define TARGET_IP	"147.32.219.248"
#define TARGET_IP	"10.0.0.95"

#define BUFFERS_LEN 1024

#define TARGET_PORT 5020
#define LOCAL_PORT 5021

#include "Psia.h"
#include "External/sha2.hpp"

static bool VerifyCRC(const Packet& inPacket)
{
	uint32_t received_crc = inPacket.CRC;
	uint32_t calculated_crc = CRC::Calculate(inPacket.Payload, inPacket.Size, CRC::CRC_32());
	// CRC::Calculate((const void*)&inPacket, 211, CRC::CRC_32());

	return received_crc == calculated_crc;
}
sha2::sha256_hash HashFile(const std::string& inFileName)
{
	sha2::sha256_hash result;

	// Open file
	FileStreamReader reader(inFileName);
	if (!reader.IsGood()) {
		FatalError("[FileStreamReader] Failed to open file '%s'\n", inFileName.c_str());
	}

	// Read data from file
	std::vector<uint8_t> data;
	data.resize(reader.GetStreamSize());
	reader.ReadData((char*)data.data(), data.size());

	// Calculate hash
	result = sha2::sha256(data.data(), data.size());

	return result;
}

bool ReceiveHash(Socket& s, std::array<uint8_t, 64>& receivedData, sha2::sha256_hash& receivedHash)
{
	// Receive the last packet
	Packet packet;
	s.RecievePacket(packet);

	// Check if the packet type is "End"
	if (packet.Type != PacketType::End) 
	{
		std::cerr << "Error: Expected last packet type to be 'End', but received type " << static_cast<int>(packet.Type) << std::endl;
		return false;
	}

	// Extract the hash from the packet at the end of the payoad
	size_t hashOffset = packet.Size - receivedHash.size();
	std::memcpy(receivedData.data(), packet.Payload + hashOffset, receivedData.size());

	return true;
}


int main()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	Socket s = Socket("Receiver");
	s.Initialize(LOCAL_PORT, 8000);

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	s.SetAddress(&addrDest);

	using namespace std::chrono_literals;

	{
		FileStreamWriter writer("Resources/SpriteSheet.png");

		Packet packet;
		packet.Type = PacketType::None;
		bool first_frame = false;
		uint32_t next_packet_id = 0;
		while (packet.Type != PacketType::End)
		{
			s.RecievePacket(packet);
			std::cout << "Received packet (ID = " << packet.ID << ")\n";

			if (packet.ID != next_packet_id)
			{
				AcknowledgePacket ack;
				ack.Acknowledgement = Acknowledgement::OK;
				ack.CRC = CRC::Calculate((const void*)&ack.Acknowledgement, 4, CRC::CRC_32());

				std::cout << "  Received packet with a wrong id (should be " << next_packet_id << " is " << packet.ID << ")" << std::endl;
				std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
				std::this_thread::sleep_for(5ms);
				s.SendAcknowledgePacket(ack);
			}
			else
			{
				// Check CRC and send acknowledgement
				bool good_crc = VerifyCRC(packet);

				AcknowledgePacket ack;

				if (good_crc)
				{
					// CRC is ok, write data from packet
					ack.Acknowledgement = Acknowledgement::OK;
					ack.CRC = CRC::Calculate((const void*)&ack.Acknowledgement, 4, CRC::CRC_32());
					writer.WritePacket(packet);
					next_packet_id++;

					std::cout << "  Received good CRC and wrote packet #" << packet.ID << "\n";
					std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
				}
				else
				{
					// CRC is wrong
					ack.Acknowledgement = Acknowledgement::BadCRC;
					ack.CRC = CRC::Calculate((const void*)&ack.Acknowledgement, 4, CRC::CRC_32());
					SetConsoleTextAttribute(hConsole, BACKGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
					std::cout << "  Received Bad CRC\n";
					std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
					SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
				}

				if (first_frame)
				{
					first_frame = false;
					std::this_thread::sleep_for(4000ms);
				}

				std::this_thread::sleep_for(5ms);
				s.SendAcknowledgePacket(ack);
			}
		}
		// Close file stream
		writer.CloseFileManually();

		// Read hash from the last packet and check it with the calculated hash
		std::array<uint8_t, 64> receivedData;
		sha2::sha256_hash receivedHash;
		if (ReceiveHash(s, receivedData, receivedHash)) 
		{
			sha2::sha256_hash calculatedHash = HashFile("Resources/SpriteSheet.png");
			if (calculatedHash == receivedHash) 
			{
				std::cout << "Hash match. Data integrity verified." << std::endl;
			}
			else 
			{
				std::cerr << "Error: Hash mismatch. Data integrity compromised." << std::endl;
			}
		}
		else 
		{
			std::cerr << "Error: Failed to receive data and hash." << std::endl;
		}

	}

	SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
	printf("Transmission terminated\n");

	// closesocket(socketS);
	std::cin.get();
}

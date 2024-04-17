// #include "stdafx.h"
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include <thread>

// #define TARGET_IP	"147.32.219.248"
#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define TARGET_PORT 5020
#define LOCAL_PORT 5021

#include "Psia.h"

static bool VerifyCRC(const Packet& inPacket)
{
	uint32_t received_crc = inPacket.CRC;
	uint32_t calculated_crc = CRC::Calculate(inPacket.Payload, inPacket.Size, CRC::CRC_32());
	// CRC::Calculate((const void*)&inPacket, 211, CRC::CRC_32());

	return received_crc == calculated_crc;
}

int main()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	Socket s = Socket("Receiver");
	s.Initialize(LOCAL_PORT, 5000);

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	s.SetAddress(&addrDest);

	using namespace std::chrono_literals;

	{
		FileStreamWriter writer("Resources/test2.jpg");

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
				ack.CRC = 123; // Calculate CRC
				ack.Acknowledgement = Acknowledgement::OK;

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
				ack.CRC = 123; // Calculate CRC

				if (good_crc)
				{
					// CRC is ok, write data from packet
					ack.Acknowledgement = Acknowledgement::OK;
					writer.WritePacket(packet);
					next_packet_id++;

					std::cout << "  Received good CRC and wrote packet #" << packet.ID << "\n";
					std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
				}
				else
				{
					// CRC is wrong
					ack.Acknowledgement = Acknowledgement::BadCRC;
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
	}

	SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
	printf("Transmission terminated\n");

	// closesocket(socketS);
	std::cin.get();
}

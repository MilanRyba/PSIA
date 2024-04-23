#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include <thread>

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define TARGET_PORT 5020
#define LOCAL_PORT 5021

#include "Psia.h"

static bool sCompareSHAHashes(const sha2::sha256_hash& inReceivedHash, const sha2::sha256_hash& inCalculatedHash)
{
	for (int i = 0; i < inReceivedHash.size(); i++)
	{
		uint8_t r = inReceivedHash[i];
		uint8_t c = inCalculatedHash[i];

		if (r != c)
			return false;
	}

	return true;
}

int main()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	Socket sock = Socket("Receiver");
	sock.Initialize(LOCAL_PORT, 8000);

	sockaddr_in addr_dest;
	addr_dest.sin_family = AF_INET;
	addr_dest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addr_dest.sin_addr.s_addr);
	sock.SetAddress(&addr_dest);

	using namespace std::chrono_literals;

	const char* file_name = "Resources/SpriteSheet.png";
	{
		FileStreamWriter writer(file_name);

		Packet packet;
		packet.Type = PacketType::None;
		uint32_t next_packet_id = 0;
		while (packet.Type != PacketType::End)
		{
			sock.RecievePacket(packet);
			std::cout << "Received packet (ID = " << packet.ID << ")\n";

			// Check if we received a correct packet
			if (packet.ID != next_packet_id)
			{
				AcknowledgementPacket ack;
				ack.CreateOK();

				std::cout << "  Received packet with a wrong id (should be " << next_packet_id << " is " << packet.ID << ")" << std::endl;
				std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
				std::this_thread::sleep_for(5ms);
				sock.SendAcknowledgementPacket(ack);

				continue;
			}
			
			// Check CRC and send acknowledgement
			AcknowledgementPacket ack;
			if (packet.TestCRC())
			{
				// CRC is ok, write data from packet
				ack.CreateOK();

				writer.WritePacket(packet);
				next_packet_id++;


				std::cout << "  Received good CRC and wrote packet #" << packet.ID << "\n";
				std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
			}
			else
			{
				// CRC is wrong
				ack.CreateBad();


				SetConsoleTextAttribute(hConsole, BACKGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
				std::cout << "  Received Bad CRC\n";
				std::cout << "  Sending Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
			}

			std::this_thread::sleep_for(5ms);
			sock.SendAcknowledgementPacket(ack);
		}

		writer.Close();

		auto hash = HashFile(file_name);
		if (sCompareSHAHashes(packet.Hash, hash))
		{
			std::cout << "Hashes are the same\n";
		}
	}

	SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
	printf("Transmission terminated\n");

	std::cin.get();
}

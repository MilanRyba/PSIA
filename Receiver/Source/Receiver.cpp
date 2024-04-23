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
	Console::sInitialize();

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

			PSIA_WARNING("---------------------------");
			PSIA_INFO("Received packet (ID = %u)", packet.ID);

			// Check if we received a correct packet
			if (packet.ID != next_packet_id)
			{
				AcknowledgementPacket ack = AcknowledgementPacket::sCreateOK();

				PSIA_ERROR("Received packet with a wrong id (should be %u is %u", next_packet_id, packet.ID);
				PSIA_TRACE("Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));
				std::this_thread::sleep_for(5ms);
				sock.SendAcknowledgementPacket(ack);

				continue;
			}
			
			// Check CRC and send acknowledgement
			AcknowledgementPacket ack;
			if (packet.TestCRC())
			{
				// CRC is ok, write data from packet
				ack = AcknowledgementPacket::sCreateOK();

				writer.WritePacket(packet);
				next_packet_id++;

				PSIA_INFO("Received a good CRC and wrote packet #%u", packet.ID);
				PSIA_TRACE("Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));
			}
			else
			{
				// CRC is wrong
				ack = AcknowledgementPacket::sCreateBad();

				PSIA_ERROR("Received incorrect CRC");
				PSIA_TRACE("Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));
			}

			std::this_thread::sleep_for(5ms);
			sock.SendAcknowledgementPacket(ack);
		}

		writer.Close();

		auto hash = HashFile(file_name);
		if (sCompareSHAHashes(packet.Hash, hash))
		{
			PSIA_INFO("Hashes are the same");
		}
	}

	PSIA_TRACE("Transmission terminated");
	std::cin.get();
}

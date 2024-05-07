#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include <thread>

#undef max

#if defined (PSIA_NET_DERPER)
	#define TARGET_IP "127.0.0.1" // IP address of Sender
	
	#define TARGET_PORT 5021
	#define LOCAL_PORT 5030
#else
	#define TARGET_IP "127.0.0.1"
	
	#define TARGET_PORT 5020
	#define LOCAL_PORT 5021
#endif

#include "Psia.h"

static uint32_t sNextPacketID = 0;

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

static void sPollPackets(Socket& inSocket, Packet& inPacket, FileStreamWriter& inWriter)
{
	using namespace std::chrono_literals;
	const char* tag = __FUNCTION__;

	const uint32_t num_of_polls = 20;
	for (uint32_t i = 0; i < num_of_polls; i++)
	{
		PSIA_WARNING("--- Next packet ID = %u ---------------------------", sNextPacketID);

		// TODO: Test if adding this function helps or not when delay is set at ridiculously high values
		// inSocket.FlushPackets();
		inPacket.Type = PacketType::Invalid;
		inSocket.RecievePacket(inPacket);

		// Timeout - did not receive anything
		if (inPacket.Type == PacketType::Invalid)
		{
			AcknowledgementPacket ack = AcknowledgementPacket::sCreateInvalid();

			PSIA_WARNING_TAG(tag, "  The received packet is invalid");
			PSIA_TRACE_TAG(tag, "  Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

			std::this_thread::sleep_for(5ms);
			inSocket.SendAcknowledgementPacket(ack);
		}

		// Test if the CRC is correct
		if (!inPacket.TestCRC())
		{
			AcknowledgementPacket ack = AcknowledgementPacket::sCreateBad();

			PSIA_ERROR_TAG(tag, "  Received incorrect CRC");
			PSIA_TRACE_TAG(tag, "  Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

			std::this_thread::sleep_for(5ms);
			inSocket.SendAcknowledgementPacket(ack);
			continue;
		}

		// Check if we received a correct packet
		if (inPacket.ID != sNextPacketID)
		{
			AcknowledgementPacket ack = AcknowledgementPacket::sCreateOK();

			PSIA_WARNING_TAG(tag, "  Received packet with a wrong id (should be %u is %u)", sNextPacketID, inPacket.ID);
			PSIA_TRACE_TAG(tag, "  Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

			std::this_thread::sleep_for(5ms);
			inSocket.SendAcknowledgementPacket(ack);

			continue;
		}

		// CRC is ok, write data from packet
		AcknowledgementPacket ack = AcknowledgementPacket::sCreateOK();

		inWriter.WritePacket(inPacket);
		sNextPacketID++;

		PSIA_INFO_TAG(tag, "  Received a good CRC and wrote packet #%u", inPacket.ID);
		PSIA_TRACE_TAG(tag, "  Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

		std::this_thread::sleep_for(5ms);
		inSocket.SendAcknowledgementPacket(ack);

		return;
	}

	PSIA_FATAL_TAG(tag, "  Did not receive good packet after %u attempts", num_of_polls);
	std::cin.get();
	exit(1);
}

int main()
{
	Console::sInitialize();

	Socket sock = Socket("Receiver");
	sock.Initialize(LOCAL_PORT, 800);

	{
		sockaddr_in addr_dest;
		addr_dest.sin_family = AF_INET;
		addr_dest.sin_port = htons(TARGET_PORT);
		InetPton(AF_INET, _T(TARGET_IP), &addr_dest.sin_addr.s_addr);
		sock.SetAddress(&addr_dest);
	}

	using namespace std::chrono_literals;

	const char* file_name = "Resources/velebil.jpg";
	{
		FileStreamWriter writer(file_name);

		PSIA_WARNING("********************************");
		PSIA_WARNING("* Waiting for the first packet *");
		PSIA_WARNING("********************************");

		Packet packet;
		packet.Type = PacketType::Payload;

		sock.SetTimeout(std::numeric_limits<uint32_t>::max());
		sock.RecievePacket(packet);
		sock.SetTimeout(800);

		// Send ack
		AcknowledgementPacket ack;
		ack = AcknowledgementPacket::sCreateOK();
		sock.SendAcknowledgementPacket(ack);

		while (packet.Type != PacketType::End)
			sPollPackets(sock, packet, writer);

		writer.Close();

		auto hash = HashFile(file_name);
		if (sCompareSHAHashes(packet.Hash, hash))
			PSIA_INFO("\n--- Hashes are the same --- :)\n");
		else
			PSIA_ERROR("\n--- Hashes are not the same --- :(\n");
	}

	PSIA_WARNING("***************************");
	PSIA_WARNING("* Transmission terminated *");
	PSIA_WARNING("***************************");

	std::cin.get();
}

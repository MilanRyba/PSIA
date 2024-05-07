#include <SDKDDKVer.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"

#include <thread>
#include <iostream>

#undef max

// Sender is the one that runs Net Derper

// xxx0 -> Port for packets
// xxx1 -> Port for acknowledgement packets

// xx2x -> From Sender/Receiver to Net Derper
// xx3x -> From NetDerper to Sender/Receiver

#if defined (PSIA_NET_DERPER)
	#define TARGET_IP "127.0.0.1"

	#define TARGET_PORT 5020
	#define LOCAL_PORT 5031
#else
	#define TARGET_IP "127.0.0.1"

	#define TARGET_PORT 5021
	#define LOCAL_PORT 5020
#endif

#include "Psia.h"

static void sPollAcknowledgements(Socket& inSocket, Packet& inPacket)
{
	using namespace std::chrono_literals;
	const char* tag = __FUNCTION__;

	PSIA_WARNING("\n--- Sending packet ID = %u ------------------------", inPacket.ID);

	const uint32_t num_of_polls = 20;
	for (uint32_t i = 0; i < num_of_polls; i++)
	{
		inSocket.SendPacket(inPacket);

		AcknowledgementPacket ack;
		inSocket.FlushAcknowledgements();
		inSocket.RecieveAcknowledgementPacket(ack);

		if (ack.Acknowledgement == EAcknowledgement::Unknown)
		{
			PSIA_WARNING_TAG(tag, "  Did not receive an AcknowledgementPacket");
			PSIA_TRACE_TAG(tag, "  Sending again #%u", inPacket.ID);
			continue;
		}

		if (!ack.TestCRC())
		{
			PSIA_ERROR_TAG(tag, "  Received AcknowledgementPacket with incorrect CRC");
			PSIA_TRACE_TAG(tag, "  Sending again #%u", inPacket.ID);
			continue;
		}

		PSIA_INFO_TAG(tag, "  Received EAcknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

		if (ack.Acknowledgement == EAcknowledgement::BadCRC)
		{
			PSIA_TRACE_TAG(tag, "  Sending again #%u", inPacket.ID);
			continue;
		}

		if (ack.Acknowledgement == EAcknowledgement::InvalidPacket)
		{
			PSIA_ERROR_TAG(tag, "  INVALID PACKET");
			continue;
		}

		// All good, move onto the next packet
		return;
	}

	PSIA_FATAL_TAG(tag, "  Sent the same packet 20 times.");
	std::cin.get();
	exit(1);
}

int main()
{
	Console::sInitialize();

	Socket sock = Socket("Sender");
	if (!sock.Initialize(LOCAL_PORT, 300))
		FatalError("[Socket] Binding error!\n");

	const char* file_name = "Resources/photo3.jpg";
	auto sha_hash = HashFile(file_name);
	{
		FileStreamReader stream(file_name);
		if (!stream.IsGood())
			FatalError("[FileStreamReader] Failed to open file '%s'\n", file_name);

		using namespace std::chrono_literals;

		// Set an optional pointer to a sockaddr structure that contains the address of the target socket.
		sockaddr_in addr_dest;
		addr_dest.sin_family = AF_INET;
		addr_dest.sin_port = htons(TARGET_PORT);
		InetPton(AF_INET, _T(TARGET_IP), &addr_dest.sin_addr.s_addr);
		sock.SetAddress(&addr_dest);

		// Determine the number of packets (of 1KB size) to send. The last packet will be sent separately
		uint64_t size = stream.GetStreamSize();
		int num_packets = (int)size / MAX_PAYLOAD_SIZE;
		int last_packet_size = size % MAX_PAYLOAD_SIZE;

		PSIA_INFO("Size of the payload: %s", BytesToString(size).c_str());
		PSIA_INFO("Number of packets to send: %i", num_packets + 1);
		PSIA_INFO("Size of the last packet: %s", BytesToString(last_packet_size).c_str());

		Packet packet;
		packet.ID = 0;
		packet.Type = PacketType::Start;
		packet.Size = MAX_PAYLOAD_SIZE;

		while (true)
		{
			std::this_thread::sleep_for(5ms);

			// We don't calculate CRC because we only care about sending a 'starting' packet 
			sock.SendPacket(packet);

			AcknowledgementPacket ack;

			sock.FlushAcknowledgements(); // Not sure if necessary
			sock.SetTimeout(1000);
			sock.RecieveAcknowledgementPacket(ack);

			if (ack.Acknowledgement != EAcknowledgement::Unknown)
				break;

			PSIA_FATAL("Did not receive ack");
		}
		sock.SetTimeout(300);

		packet.Type = PacketType::Payload;
		for (int i = 0; i < num_packets; i++)
		{
			// Read data into the packet
			stream.ReadData((char*)packet.Payload, packet.Size);
			packet.CRC = Packet::sCalculateCRC(packet);

			// Wait for the receiver to be ready
			std::this_thread::sleep_for(5ms);
			sPollAcknowledgements(sock, packet);
			packet.ID++;
		}

		// Send the last packet manually
		packet.Hash = sha_hash;

		for (uint8_t c : packet.Hash)
		{
			std::cout << c;
		}
		std::cout << "\n";

		packet.Type = PacketType::End;
		packet.Size = last_packet_size;
		stream.ReadData((char*)packet.Payload, packet.Size);
		packet.CRC = Packet::sCalculateCRC(packet);

		sock.FlushAcknowledgements();
		sock.SendPacket(packet);
		sPollAcknowledgements(sock, packet);
	}

	std::cin.get();
}

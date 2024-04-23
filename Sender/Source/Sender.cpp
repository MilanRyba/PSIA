#include <SDKDDKVer.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"

#include <thread>
#include <iostream>

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define TARGET_PORT 5021
#define LOCAL_PORT 5020

#include "Psia.h"
#include "External/CRC.h"

static void sPollAcknowledgements(Socket& inSocket, Packet& inPacket)
{
	const char* tag = "sPollAcknowledgements";

	using namespace std::chrono_literals;

	AcknowledgementPacket ack;
	inSocket.RecieveAcknowledgementPacket(ack);

	// std::cout << "  Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
	PSIA_TRACE_TAG(tag, "Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));
	
	// If the CRC of AcknowledgementPacket is incorrect, 
	// treat it the same as other errors and send the original packet again
	if (!ack.TestCRC())
	{
		PSIA_WARNING_TAG(tag, "Received AcknowledgementPacket with incorrect CRC");
		ack.Acknowledgement = EAcknowledgement::BadCRC;
	}

	int num_packets_sent = 0;
	while (ack.Acknowledgement == EAcknowledgement::BadCRC || ack.Acknowledgement == EAcknowledgement::Unknown)
	{
		// If we sent the same packet more than 10 times, terminate
		if (num_packets_sent > 10)
			FatalError("[%s] Sent the same packet 10 times.", __FUNCTION__);

		PSIA_INFO_TAG(tag, "Sending again #%u", inPacket.ID);

		inPacket.CalculateCRC();
		PSIA_TRACE_TAG(tag, "New CRC = %u", inPacket.CRC);
		
		std::this_thread::sleep_for(5ms);

		if (ack.Acknowledgement == EAcknowledgement::Unknown)
		{
			// Acknowledgement::Unknown means that we waited for too long and didn't receive an acknowledgement. So we send the same packet
			// however when we look for the acknowledgement we get the one that we wait
			inSocket.FlushAcknowledgements();
		}

		inSocket.SendPacket(inPacket);
		inSocket.RecieveAcknowledgementPacket(ack);

		if (!ack.TestCRC())
		{
			PSIA_WARNING_TAG(tag, "Received AcknowledgementPacket with incorrect CRC (%s)", AcknowledgementToString(ack.Acknowledgement));
			ack.Acknowledgement = EAcknowledgement::BadCRC;
		}

		PSIA_TRACE_TAG(tag, "Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

		num_packets_sent++;
	}

	PSIA_INFO("Exited acknowledgement polling");
}

int main()
{
	Console::sInitialize();

	// Name 'socket' is taken by a function...
	Socket sock = Socket("Sender");
	if (!sock.Initialize(LOCAL_PORT, 3000))
		FatalError("[Socket] Binding error!\n");

	const char* file_name = "Resources/BRDF_LUT.png";
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
		packet.Size = MAX_PAYLOAD_SIZE;
		for (int i = 0; i < num_packets; i++)
		{
			// Read data into the packet
			stream.ReadData((char*)packet.Payload, packet.Size);
			packet.CalculateCRC();

			PSIA_WARNING("---------------------------");
			PSIA_INFO("Sending packet #%u", packet.ID);
			PSIA_TRACE("CRC = %u", packet.CRC);

			// Wait for the receiver to be ready
			std::this_thread::sleep_for(5ms);
			sock.FlushAcknowledgements();
			sock.SendPacket(packet);

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
		packet.CalculateCRC();

		sock.FlushAcknowledgements();
		sock.SendPacket(packet);
		// PollAcknowledgements(s, packet);
	}

	std::cin.get();
}

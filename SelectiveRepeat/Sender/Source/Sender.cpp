#include <SDKDDKVer.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"

#include <thread>
#include <iostream>
#include <unordered_map>

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
	// #define TARGET_IP "246.12.165.112"
	#define TARGET_IP "127.0.0.1"

	#define TARGET_PORT 5021
	#define LOCAL_PORT 5020
#endif

#include "Psia.h"

/*
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
		inSocket.ReceiveAcknowledgementPacket(ack);

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
	{
		for (int i = 0; i < WINDOW_SIZE; i++)
		{
			Packet packet;
			packet.Type = PacketType::Payload;
			packet.ID = i;
			packet.Size = MAX_PAYLOAD_SIZE;
			stream.ReadData((char*)packet.Payload, packet.Size);
			packet.CRC = Packet::sCalculateCRC(packet);
			mBufferdPackets[i] = packet;
		}

		old_packet.Type = PacketType::Payload;
		for (int i = 0; i < num_packets; i++)
		{
			// Read data into the packet
			stream.ReadData((char*)old_packet.Payload, old_packet.Size);
			old_packet.CRC = Packet::sCalculateCRC(old_packet);

			// Wait for the receiver to be ready
			std::this_thread::sleep_for(5ms);
			sPollAcknowledgements(sock, old_packet);
			old_packet.ID++;
		}

		// Send the last packet manually
		old_packet.Hash = sha_hash;

		for (uint8_t c : old_packet.Hash)
		{
			std::cout << c;
		}
		std::cout << "\n";

		old_packet.Type = PacketType::End;
		old_packet.Size = last_packet_size;
		stream.ReadData((char*)old_packet.Payload, old_packet.Size);
		old_packet.CRC = Packet::sCalculateCRC(old_packet);

		sock.FlushAcknowledgements();
		sock.SendPacket(old_packet);
		sPollAcknowledgements(sock, old_packet);
	}

	std::cin.get();
}
*/

/****************
*   REFERENCE   *
****************/

#define WINDOW_SIZE 5

static std::unordered_map<uint32_t, Packet> sBufferedPackets;
static uint32_t sBasePacket;
static uint32_t sNumPackets;		// Number of packets to send
static uint32_t sLastPacketSize;	// Size of the last packet
static sha2::sha256_hash sHash;		// File hash

static bool sAcknowledgements[256];

static void sSendWindow(Socket& inSocket, FileStreamReader& inStream)
{
	const char* tag = __FUNCTION__;

	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		uint32_t packet_id = sBasePacket + i;

		// Check if packet with this ID has already been acknowledged
		if (sAcknowledgements[packet_id])
			continue;

		// Find packet with this ID inside the map
		if (sBufferedPackets.find(packet_id) != sBufferedPackets.end())
		{
			PSIA_TRACE_TAG(tag, "  Found packet #%u inside map and sending", packet_id);

			const Packet& packet = sBufferedPackets.at(packet_id);
			inSocket.SendPacket(packet);
			continue;
		}

		/*
		* xx Properly create the last packet (correct size, type, hash)
		* xx Construct the packet in-place
		* Better logging
		* Bitfield for sAcknowledgements and sReceived
		* Log error messages inside sending functions
		*/

		// Construct the packet in-place
		sBufferedPackets.emplace(packet_id, Packet());

		Packet& packet = sBufferedPackets[packet_id];
		packet.ID = packet_id;
		packet.Type = packet_id == sNumPackets ? PacketType::End : PacketType::Payload;
		packet.Size = packet_id == sNumPackets ? sLastPacketSize : MAX_PAYLOAD_SIZE;
		inStream.ReadData((char*)packet.Payload, packet.Size);
		packet.Hash = sHash;
		packet.CRC = Packet::sCalculateCRC(packet);

		inSocket.SendPacket(packet);

		PSIA_INFO_TAG(tag, "Constructed packet #%u and sending", packet_id);
	}
}

int main()
{
	Console::sInitialize();

	sBasePacket = 0;
	memset(sAcknowledgements, 0, sizeof(sAcknowledgements));

	Socket sock = Socket("Sender");
	if (!sock.Initialize(LOCAL_PORT, 300))
		FatalError("[Socket] Binding error!\n");

	const char* file_name = "Resources/BRDF_LUT.png";
	sHash = HashFile(file_name);

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
	sNumPackets = (uint32_t)size / MAX_PAYLOAD_SIZE;
	sLastPacketSize = size % MAX_PAYLOAD_SIZE;

	PSIA_INFO("Size of the payload: %s", BytesToString(size).c_str());
	PSIA_INFO("Number of packets to send: %i", sNumPackets + 1);
	PSIA_INFO("Size of the last packet: %s", BytesToString(sLastPacketSize).c_str());

	// TODO: Move into a function
	PSIA_WARNING("***************************");
	PSIA_WARNING("* Sending starting packet *");
	PSIA_WARNING("***************************");
	Packet packet;
	packet.ID = 0;
	packet.Type = PacketType::Start;
	packet.Size = MAX_PAYLOAD_SIZE;
	sock.SetTimeout(1000);
	while (true)
	{
		std::this_thread::sleep_for(5ms);

		// We don't calculate CRC because we only care about sending a 'starting' packet
		sock.SendPacket(packet);

		AcknowledgementPacket ack;
		sock.ReceiveAcknowledgementPacket(ack);

		if (ack.Acknowledgement != EAcknowledgement::Unknown)
			break;

		PSIA_WARNING("Did not receive acknowledgement");
	}
	sock.SetTimeout(300);

	while (true)
	{
		sSendWindow(sock, stream);

		while (true)
		{
			using namespace std::chrono_literals;
			const char* tag = __FUNCTION__;

			AcknowledgementPacket ack;
			sock.ReceiveAcknowledgementPacket(ack);

			// Timeout - break and send window
			if (ack.Acknowledgement == EAcknowledgement::Unknown)
			{
				PSIA_WARNING_TAG(tag, "  Timeout - Sending window");
				break;
			}

			if (!ack.TestCRC())
			{
				PSIA_ERROR_TAG(tag, "  Received AcknowledgementPacket with incorrect CRC");
				continue;
			}

			PSIA_INFO_TAG(tag, "  Received AcknowledgementPacket #%u:", ack.ID);
			PSIA_INFO_TAG(tag, "    EAcknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

			if (ack.Acknowledgement == EAcknowledgement::BadCRC)
				continue;

			if (ack.ID >= sBasePacket)
			{
				// Mark this packet as successfully delivered
				sAcknowledgements[ack.ID] = true;

				while (sAcknowledgements[sBasePacket])
				{
					// Remove packet with this ID from the map
					sBufferedPackets.erase(sBasePacket);

					sBasePacket++;
					PSIA_TRACE_TAG(tag, "  Moved sBasePacket to %d", sBasePacket);
				}
			}
		}

		if (sBasePacket >= sNumPackets)
			break;
	}

	std::cin.get();
}

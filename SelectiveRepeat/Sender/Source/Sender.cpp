#include <SDKDDKVer.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"

#include <thread>

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

#include "Sender.h"

using namespace std::chrono_literals;

/*
* TODO:
* Better logging
* Bitfield for mAcknowledgements and mReceived
* If Sender sends the last 5 packets multiple times and does not receive ack,
*  Receiver probably already wrote it and left the chat, so leave as well
*/

int main()
{
	Console::sInitialize();

	// Create socket (for some reason it needs to be created like this and cannot be Sender's member)
	Socket sock = Socket("Sender");
	if (!sock.Initialize(LOCAL_PORT, 300))
		FatalError("[Socket] Binding error!\n");

	// Set an optional pointer to a sockaddr structure that contains the address of the target socket.
	sockaddr_in addr_dest;
	addr_dest.sin_family = AF_INET;
	addr_dest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addr_dest.sin_addr.s_addr);
	sock.SetAddress(&addr_dest);

	const char* file_name = "Resources/BRDF_LUT.png";
	Sender sender(file_name);
	sender.Send(sock);

	std::cin.get();
}

Sender::Sender(const char* inFileName)
	: mStream(inFileName)
{
	if (!mStream.IsGood())
		FatalError("[FileStreamReader] Failed to open file '%s'\n", inFileName);

	// Determine the number of packets (of 1KB size) to send
	uint64_t size = mStream.GetStreamSize();
	mNumPackets = (uint32_t)size / MAX_PAYLOAD_SIZE;
	mNumPackets++;
	mLastPacketSize = size % MAX_PAYLOAD_SIZE;

	PSIA_INFO("Size of the payload: %s", BytesToString(size).c_str());
	PSIA_INFO("Number of packets to send: %i", mNumPackets);
	PSIA_INFO("Size of the last packet: %s", BytesToString(mLastPacketSize).c_str());

	memset(mAcknowledgements, 0, sizeof(mAcknowledgements));

	// Wait 5ms, just to be sure
	std::this_thread::sleep_for(5ms);
}

void Sender::Send(Socket& inSocket)
{
	const char* tag = __FUNCTION__;

	while (true)
	{
		SendWindow(inSocket);

		while (true)
		{
			AcknowledgementPacket ack;
			inSocket.ReceiveAcknowledgementPacket(ack);

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

			if (ack.ID >= mBasePacket)
			{
				// Mark this packet as successfully delivered
				mAcknowledgements[ack.ID] = true;

				while (mAcknowledgements[mBasePacket])
				{
					// Remove packet with this ID from the map
					mBufferedPackets.erase(mBasePacket);

					mBasePacket++;
					PSIA_TRACE_TAG(tag, "  Moved sBasePacket to %d", mBasePacket);
				}
			}
		}

		if (mBasePacket > mNumPackets)
			break;
	}

	PSIA_TRACE("");
	PSIA_WARNING("***************************");
	PSIA_WARNING("* Transmission terminated *");
	PSIA_WARNING("***************************");
}

void Sender::SendWindow(Socket& inSocket)
{
	const char* tag = __FUNCTION__;

	PSIA_WARNING("--- Sending window, Base Packet = %u", mBasePacket);

	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		uint32_t packet_id = mBasePacket + i;

		if (packet_id > mNumPackets)
			break;

		// Check if packet with this ID has already been acknowledged
		if (mAcknowledgements[packet_id])
			continue;

		// Find packet with this ID inside the map
		if (mBufferedPackets.find(packet_id) != mBufferedPackets.end())
		{
			PSIA_TRACE_TAG(tag, "  Found packet #%u inside map", packet_id);

			const Packet& packet = mBufferedPackets.at(packet_id);
			inSocket.SendPacket(packet);
			continue;
		}

		// Construct the packet in-place
		mBufferedPackets.emplace(packet_id, Packet());
		Packet& packet = mBufferedPackets[packet_id];

		PSIA_INFO_TAG(tag, "Constructed packet #%u", packet_id);

		if (packet_id == 0)
			CreateStartPacket(packet);
		else if (packet_id == mNumPackets)
			CreateEndPacket(packet);
		else
			CreatePayloadPacket(packet, packet_id);

		inSocket.SendPacket(packet);
	}
}

void Sender::CreateStartPacket(Packet& inPacket)
{
	inPacket.Type = PacketType::Start;
	inPacket.ID = 0;
	inPacket.Size = 0;
	inPacket.Hash = HashFile(mStream.GetFileName());
	inPacket.CRC = Packet::sCalculateCRC(inPacket);
}

void Sender::CreatePayloadPacket(Packet& inPacket, uint32_t inID)
{
	inPacket.Type = PacketType::Payload;
	inPacket.ID = inID;
	inPacket.Size = MAX_PAYLOAD_SIZE;
	mStream.ReadData((char*)inPacket.Payload, inPacket.Size);
	inPacket.CRC = Packet::sCalculateCRC(inPacket);
}

void Sender::CreateEndPacket(Packet& inPacket)
{
	inPacket.Type = PacketType::End;
	inPacket.ID = mNumPackets;
	inPacket.Size = mLastPacketSize;
	mStream.ReadData((char*)inPacket.Payload, inPacket.Size);
	inPacket.CRC = Packet::sCalculateCRC(inPacket);
}

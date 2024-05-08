#include <SDKDDKVer.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"

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

#include "Receiver.h"

using namespace std::chrono_literals;

int main()
{
	Console::sInitialize();

	const char* file_name = "Resources/BRDF_LUT.png";
	Receiver receiver(file_name);
	receiver.Receive();

	std::cin.get();
}

Receiver::Receiver(const char* inFileName)
	: mSocket("Receiver"), mStream(inFileName)
{
	if (!mSocket.Initialize(LOCAL_PORT, 800))
		FatalError("[Socket] Binding error!\n");

	// Set an optional pointer to a sockaddr structure that contains the address of the target socket.
	sockaddr_in addr_dest;
	addr_dest.sin_family = AF_INET;
	addr_dest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addr_dest.sin_addr.s_addr);
	mSocket.SetAddress(&addr_dest);

	memset(mReceived, 0, sizeof(mReceived));
}

void Receiver::Receive()
{
	const char* tag = __FUNCTION__;
	sha2::sha256_hash sender_hash;

	Packet packet;
	bool running = true;
	while (running)
	{
		// Reset packet type
		packet.Type = PacketType::Invalid;

		mSocket.ReceivePacket(packet);

		// Timeout - return to listening
		if (packet.Type == PacketType::Invalid)
			continue;

		// Test if the CRC is correct
		if (!packet.TestCRC())
		{
			// For AcknowledgementPacket's with BadCRC, the ID does not matter
			AcknowledgementPacket ack = AcknowledgementPacket::sCreateBad(mBasePacket);

			PSIA_ERROR_TAG(tag, "  Received incorrect CRC");
			PSIA_TRACE_TAG(tag, "  Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

			mSocket.SendAcknowledgementPacket(ack);
			continue;
		}

		AcknowledgementPacket ack = AcknowledgementPacket::sCreateOK(packet.ID);

		PSIA_INFO_TAG(tag, "  Received packet #%u with good CRC", packet.ID);
		PSIA_TRACE_TAG(tag, "  Sending Acknowledgement = %s", AcknowledgementToString(ack.Acknowledgement));

		mReceived[packet.ID] = true;
		mSocket.SendAcknowledgementPacket(ack);

		if (packet.ID == mBasePacket)
		{
			// Write the packet if it is the base one
			if (packet.Type != PacketType::Start)
				mStream.WritePacket(packet);
			else
				sender_hash = packet.Hash;

			if (packet.Type == PacketType::End)
				running = false;
		}
		else
			mBufferedPackets[packet.ID] = packet;

		while (mReceived[mBasePacket])
		{
			if (mBufferedPackets.find(mBasePacket) != mBufferedPackets.end())
			{
				const Packet& buffered_packet = mBufferedPackets.at(mBasePacket);
				mStream.WritePacket(buffered_packet);

				if (buffered_packet.Type == PacketType::End)
					running = false;

				mBufferedPackets.erase(mBasePacket);
			}

			mBasePacket++;
			PSIA_TRACE_TAG(tag, "  Moved window to %d", mBasePacket);
		}
	}

	mStream.Close();

	auto hash = HashFile(mStream.GetFileName());
	if (CompareSHAHashes(sender_hash, hash))
		PSIA_INFO("\n--- Hashes are the same --- :)\n");
	else
		PSIA_ERROR("\n--- Hashes are not the same --- :(\n");

	PSIA_WARNING("***************************");
	PSIA_WARNING("* Transmission terminated *");
	PSIA_WARNING("***************************");
}

bool Receiver::CompareSHAHashes(const sha2::sha256_hash& inReceivedHash, const sha2::sha256_hash& inCalculatedHash)
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

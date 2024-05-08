#pragma once
#include "Psia.h"

#include <unordered_map>

#define WINDOW_SIZE 5

class Sender
{
public:
	Sender(const char* inFileName);
	void Send(Socket& inSocket);

private:
	void SendWindow(Socket& inSocket);

	void CreateStartPacket(Packet& inPacket);
	void CreatePayloadPacket(Packet& inPacket, uint32_t inID);
	void CreateEndPacket(Packet& inPacket);

	// Socket mSocket;
	FileStreamReader mStream;
	std::unordered_map<uint32_t, Packet> mBufferedPackets;

	uint32_t mNumPackets;
	uint32_t mLastPacketSize;
	uint32_t mBasePacket = 0;

	// TODO: Convert to Bitfield
	bool mAcknowledgements[1024];
};

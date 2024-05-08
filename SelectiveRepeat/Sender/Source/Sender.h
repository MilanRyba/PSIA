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

	FileStreamReader mStream;
	std::unordered_map<uint32_t, Packet> mBufferedPackets;
	BitField<1024> mAcknowledgements;

	uint32_t mNumPackets;
	uint32_t mLastPacketSize;
	uint32_t mBasePacket = 0;
	uint32_t mCountdown = 0;
};

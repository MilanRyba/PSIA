#pragma once
#include "Psia.h"

#include <map>

#define WINDOW_SIZE 5

class Receiver
{
public:
	Receiver(const char* inFileName);
	void Receive();

private:
	bool CompareSHAHashes(const sha2::sha256_hash& inReceivedHash, const sha2::sha256_hash& inCalculatedHash);

	Socket mSocket;
	FileStreamWriter mStream;
	std::map<uint32_t, Packet> mBufferedPackets;

	uint32_t mBasePacket = 0;

	// TODO: Convert to Bitfield
	bool mReceived[1024];
};
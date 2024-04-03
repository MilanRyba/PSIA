#pragma once

enum class PacketType
{
	None = 0,
	End, // Indicates the last packet
};

#define MAX_PAYLOAD_SIZE 1024

struct Packet
{
	// ???
	// char FileName[7];
	// size_t FileSize = 0;

	PacketType Type; // What type o packet is this
	uint64_t Size; // The size of Payload
	uint8_t Payload[MAX_PAYLOAD_SIZE];
};
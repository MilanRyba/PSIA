#pragma once
#include "External/CRC.h"
#include <array>

enum class PacketType
{
	None = 0,
	Start,
	End, // Indicates the last packet
};

#define MAX_PAYLOAD_SIZE 1024

struct Packet
{
	PacketType Type; // What type o packet is this
	uint32_t CRC;

	uint32_t ID; // When ack packet goes missing
	uint64_t Size; // The size of Payload
	uint8_t Payload[MAX_PAYLOAD_SIZE];

	std::array<uint8_t, 64> Hash; // 64 byte hash

	inline void CalculateCRC() { CRC = CRC::Calculate(Payload, Size, CRC::CRC_32()); }
};

enum class Acknowledgement
{
	Unknown, // AcknowledgementPacket was lost
	OK,
	BadCRC,
	WrongPacketID,
};

inline const char* AcknowledgementToString(const Acknowledgement inAcknowledgement)
{
	switch (inAcknowledgement)
	{
	case Acknowledgement::Unknown:			return "Unknown";
	case Acknowledgement::OK:				return "OK";
	case Acknowledgement::BadCRC:			return "BadCRC";
	case Acknowledgement::WrongPacketID:	return "WrongPacketID";
	default: return "[AcknowledgementToString] Wrong enum!";
	}
}

struct AcknowledgePacket
{
	Acknowledgement Acknowledgement = Acknowledgement::Unknown;
	uint32_t CRC;
};

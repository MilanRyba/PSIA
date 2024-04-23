#pragma once
#include "External/CRC.h"
#include "External/sha2.hpp"

#include <array>
#include <iostream>

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
	sha2::sha256_hash Hash;

	uint32_t ID; // When ack packet goes missing
	uint64_t Size; // The size of Payload
	uint8_t Payload[MAX_PAYLOAD_SIZE];

	inline void CalculateCRC() { CRC = CRC::Calculate(Payload, Size, CRC::CRC_32()); }

	inline bool TestCRC()
	{
		uint32_t calculated_crc = CRC::Calculate(Payload, Size, CRC::CRC_32());
		return calculated_crc == CRC;
	}
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

struct AcknowledgementPacket
{
	Acknowledgement Acknowledgement = Acknowledgement::Unknown;
	uint32_t CRC = UINT32_MAX;

	inline void CreateOK()
	{
		Acknowledgement = Acknowledgement::OK;
		CalculateCRC();
	}

	inline void CreateBad()
	{
		Acknowledgement = Acknowledgement::BadCRC;
		CalculateCRC();
	}

	inline void CalculateCRC()
	{
		const char* ack_string = AcknowledgementToString(Acknowledgement);
		CRC = CRC::Calculate(ack_string, sizeof(ack_string), CRC::CRC_32());
	}
};

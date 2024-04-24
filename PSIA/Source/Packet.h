#pragma once
#include "External/CRC.h"

// Suppres warning 'unary minus operator applied to unsigned type, result still unsigned'
__pragma(warning(push))
__pragma(warning(disable : 4146))
#include "External/sha2.hpp"
__pragma(warning(pop))

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

enum class EAcknowledgement
{
	Unknown, // AcknowledgementPacket was lost
	OK,
	BadCRC,
	WrongPacketID,
};

inline const char* AcknowledgementToString(const EAcknowledgement inAcknowledgement)
{
	switch (inAcknowledgement)
	{
	case EAcknowledgement::Unknown:			return "Unknown";
	case EAcknowledgement::OK:				return "OK";
	case EAcknowledgement::BadCRC:			return "BadCRC";
	case EAcknowledgement::WrongPacketID:	return "WrongPacketID";
	default: return "[AcknowledgementToString] Wrong enum!";
	}
}

struct AcknowledgementPacket
{
	EAcknowledgement Acknowledgement = EAcknowledgement::Unknown;
	uint32_t CRC = UINT32_MAX;

	inline static AcknowledgementPacket sCreateOK()
	{
		// Conservative method :)
		return AcknowledgementPacket(EAcknowledgement::OK, sCalculateCRC(EAcknowledgement::OK));

		// AcknowledgementPacket packet;
		// packet.Acknowledgement = EAcknowledgement::OK;
		// packet.CRC = sCalculateCRC(packet.Acknowledgement);
		// return packet;
	}

	inline static AcknowledgementPacket sCreateBad()
	{
		return AcknowledgementPacket(EAcknowledgement::BadCRC, sCalculateCRC(EAcknowledgement::BadCRC));

		// AcknowledgementPacket packet;
		// packet.Acknowledgement = EAcknowledgement::BadCRC;
		// packet.CRC = sCalculateCRC(packet.Acknowledgement);
		// return packet;
	}

	inline static uint32_t sCalculateCRC(EAcknowledgement inAcknowledgement)
	{
		// const char* ack_string = AcknowledgementToString(inAcknowledgement);
		return CRC::Calculate(&inAcknowledgement, sizeof(inAcknowledgement), CRC::CRC_32());
	}

	inline bool TestCRC()
	{
		// const char* ack_string = AcknowledgementToString(inAcknowledgement);
		uint32_t calculated_crc = sCalculateCRC(Acknowledgement);
		return calculated_crc == CRC;
	}
};

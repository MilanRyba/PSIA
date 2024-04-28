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
	Invalid = 0,	// Receiver will discard this packet
	Payload,
	Start,
	End,			// Indicates the last packet
};

#define MAX_PAYLOAD_SIZE 1024

struct Packet
{
	PacketType								Type = PacketType::Invalid;
	uint32_t								CRC = std::numeric_limits<uint32_t>::max();
	sha2::sha256_hash						Hash = { 0 };

	uint32_t								ID = std::numeric_limits<uint32_t>::max();
	uint64_t								Size = std::numeric_limits<uint64_t>::max();
	uint8_t									Payload[MAX_PAYLOAD_SIZE];

	// Calculates CRC from this packet
	// Call after all other packet fields have been set
	inline static uint32_t					sCalculateCRC(Packet& inPacket)
	{
		uint32_t original_crc = inPacket.CRC;

		inPacket.CRC = 0;
		uint32_t crc = CRC::Calculate(&inPacket, sizeof(Packet), CRC::CRC_32());

		inPacket.CRC = original_crc;
		return crc;
	}

	inline bool								TestCRC()
	{
		uint32_t received_crc = CRC;
		uint32_t calculated_crc = sCalculateCRC(*this);

		return calculated_crc == received_crc;
	}
};

enum class EAcknowledgement
{
	Unknown,		// AcknowledgementPacket was lost
	OK,
	BadCRC,
	InvalidPacket,  // The Receiver received invalid packet, likely from timeout
	WrongPacketID,
};

inline const char* AcknowledgementToString(const EAcknowledgement inAcknowledgement)
{
	switch (inAcknowledgement)
	{
	case EAcknowledgement::Unknown:			return "Unknown";
	case EAcknowledgement::OK:				return "OK";
	case EAcknowledgement::BadCRC:			return "BadCRC";
	case EAcknowledgement::InvalidPacket:   return "InvalidPacket";
	case EAcknowledgement::WrongPacketID:	return "WrongPacketID";
	default: return "[AcknowledgementToString] Wrong enum!";
	}
}

struct AcknowledgementPacket
{
	EAcknowledgement						Acknowledgement = EAcknowledgement::Unknown;
	uint32_t								CRC = std::numeric_limits<uint32_t>::max();

	inline static AcknowledgementPacket		sCreateOK()
	{
		AcknowledgementPacket ack;
		ack.Acknowledgement = EAcknowledgement::OK;
		ack.CRC = sCalculateCRC(ack);
		return ack;
	}

	inline static AcknowledgementPacket		sCreateBad()
	{
		AcknowledgementPacket ack;
		ack.Acknowledgement = EAcknowledgement::BadCRC;
		ack.CRC = sCalculateCRC(ack);
		return ack;
	}

	inline static AcknowledgementPacket		sCreateInvalid()
	{
		AcknowledgementPacket ack;
		ack.Acknowledgement = EAcknowledgement::InvalidPacket;
		ack.CRC = sCalculateCRC(ack);
		return ack;
	}

	inline static uint32_t					sCalculateCRC(AcknowledgementPacket& inAcknowledgement)
	{
		uint32_t original_crc = inAcknowledgement.CRC;

		inAcknowledgement.CRC = 0;
		uint32_t crc = CRC::Calculate(&inAcknowledgement, sizeof(AcknowledgementPacket), CRC::CRC_32());

		inAcknowledgement.CRC = original_crc;
		return crc;
	}

	inline bool								TestCRC()
	{
		uint32_t received_crc = CRC;		
		uint32_t calculated_crc = sCalculateCRC(*this);

		return calculated_crc == received_crc;
	}
};

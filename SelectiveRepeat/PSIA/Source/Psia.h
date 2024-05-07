#pragma once
#include <vector>
#include <random>

#include "Packet.h"
#include "FileStream.h"
#include "Socket.h"
#include "Console.h"

inline std::string BytesToString(uint64_t inBytes)
{
	constexpr float BytesToKiloBytes = 1.0f / (1 << 10);
	constexpr float BytesToMegaBytes = 1.0f / (1 << 20);
	constexpr float BytesToGigaBytes = 1.0f / (1 << 30);

	constexpr uint32_t KiloBytesToBytes = 1 << 10;
	constexpr uint32_t MegaBytesToBytes = 1 << 20;
	constexpr uint32_t GigaBytesToBytes = 1 << 30;

	char buffer[32 + 1]{};

	if (inBytes >= GigaBytesToBytes)
		snprintf(buffer, 32, "%.2f GB", (float)inBytes * BytesToGigaBytes);
	else if (inBytes >= MegaBytesToBytes)
		snprintf(buffer, 32, "%.2f MB", (float)inBytes * BytesToMegaBytes);
	else if (inBytes >= KiloBytesToBytes)
		snprintf(buffer, 32, "%.2f KB", (float)inBytes * BytesToKiloBytes);
	else
		snprintf(buffer, 32, "%.2f B", (float)inBytes);

	return std::string(buffer);
}

inline void FatalError [[noreturn]] (const char* inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);

	printf(buffer);
	OutputDebugStringA(buffer);
	exit(1);
}

inline sha2::sha256_hash HashFile(const std::string& inFileName)
{
	FileStreamReader reader(inFileName);
	if (!reader.IsGood())
	{
		FatalError("[FileStreamReader] Failed to open file '%s'\n", inFileName.c_str());
	}

	// Read data from file
	std::vector<uint8_t> data;
	data.resize(reader.GetStreamSize());
	reader.ReadData((char*)data.data(), data.size());

	// Calculate hash
	return sha2::sha256(data.data(), data.size());
}

inline uint32_t sRandom(uint32_t inSeed)
{
	std::default_random_engine random(inSeed);
	std::uniform_int_distribution<uint32_t> size;
	return size(random);
}

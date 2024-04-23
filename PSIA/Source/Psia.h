#pragma once
#include <vector>

// Suppres warning 'unary minus operator applied to unsigned type, result still unsigned'
__pragma(warning(push))
__pragma(warning(disable : 4146))
#include <External/sha2.hpp>
__pragma(warning(pop))

#include "Packet.h"
#include "FileStream.h"
#include "Socket.h"

inline std::string StringFormat(const char* inFMT, ...)
{
	char buffer[1024];

	// A complete object type (in practice, a unique built-in type or char*)
	// suitable for holding the information needed by the macros such as va_start
	va_list list;

	// Enables access to variadic function arguments
	// The second parameter is the named parameter preceding the first variable parameter
	va_start(list, inFMT);

	// Converts data from 'list' to character string equivalents and writes the results to 'buffer'
	vsnprintf(buffer, sizeof(buffer), inFMT, list);

	// Ends traversal of the variadic function arguments
	va_end(list);

	return std::string(buffer);
}

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

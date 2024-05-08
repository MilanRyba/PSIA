#include "FileStream.h"
#include "Packet.h"

FileStreamWriter::FileStreamWriter(const std::string& inPath)
	: mPath(inPath)
{
	mStream = std::ofstream(inPath, std::ios::binary | std::ios::out);
}

FileStreamWriter::~FileStreamWriter()
{
	mStream.close();
}

void FileStreamWriter::WriteData(const char* inData, size_t inSize)
{
	mStream.write(inData, inSize);
}

void FileStreamWriter::WritePacket(const Packet& inPacket)
{
	mStream.write((const char*)inPacket.Payload, inPacket.Size);
}

FileStreamReader::FileStreamReader(const std::string& inPath)
	: mPath(inPath)
{
	mStream = std::ifstream(inPath, std::ios::binary | std::ios::in);
}

FileStreamReader::~FileStreamReader()
{
	mStream.close();
}

uint64_t FileStreamReader::GetStreamSize()
{
	// Store current stream position
	std::streampos current = mStream.tellg();

	mStream.seekg(0, std::ios::end);
	std::streampos end = mStream.tellg();
	mStream.seekg(0, std::ios::beg);
	uint64_t length = end - mStream.tellg();

	// Return to the original position
	mStream.seekg(current);

	return length;
}

void FileStreamReader::ReadData(char* inDest, size_t inSize)
{
	mStream.read(inDest, inSize);
}


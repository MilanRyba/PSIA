#pragma once
#include <filesystem>
#include <fstream>

struct Packet;

class FileStreamWriter
{
public:
	FileStreamWriter(const std::filesystem::path& inPath);
	FileStreamWriter(const FileStreamWriter&) = delete;
	~FileStreamWriter();

	bool IsGood() const { return mStream.good(); }
	uint64_t GetStreamPosition() { return mStream.tellp(); }
	void SetStreamPosition(uint64_t inPosition) { mStream.seekp(inPosition); }
	void WriteData(const char* inData, size_t inSize);
	void WritePacket(const Packet& inPacket);

private:
	std::filesystem::path mPath;
	std::ofstream mStream;
};

class FileStreamReader
{
public:
	FileStreamReader(const std::filesystem::path& inPath);
	FileStreamReader(const FileStreamReader&) = delete;
	~FileStreamReader();

	bool IsGood() const { return mStream.good(); }
	uint64_t GetStreamPosition() { return mStream.tellg(); }
	void SetStreamPosition(uint64_t inPosition) { mStream.seekg(inPosition); }
	uint64_t GetStreamSize();

	void ReadData(char* inDest, size_t inSize);

private:
	std::filesystem::path mPath;
	std::ifstream mStream;
};

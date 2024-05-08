#pragma once
#include <filesystem>
#include <fstream>

struct Packet;

class FileStreamWriter
{
public:
	FileStreamWriter(const std::string& inPath);
	FileStreamWriter(const FileStreamWriter&) = delete;
	~FileStreamWriter();

	bool IsGood() const { return mStream.good(); }
	void Close() { mStream.close(); }

	uint64_t GetStreamPosition() { return mStream.tellp(); }
	void SetStreamPosition(uint64_t inPosition) { mStream.seekp(inPosition); }
	const char* GetFileName() const { return mPath.c_str(); }

	void WriteData(const char* inData, size_t inSize);
	void WritePacket(const Packet& inPacket);

private:
	std::string mPath;
	std::ofstream mStream;
};

class FileStreamReader
{
public:
	FileStreamReader(const std::string& inPath);
	FileStreamReader(const FileStreamReader&) = delete;
	~FileStreamReader();

	bool IsGood() const { return mStream.good(); }
	void Close() { mStream.close(); }

	uint64_t GetStreamPosition() { return mStream.tellg(); }
	void SetStreamPosition(uint64_t inPosition) { mStream.seekg(inPosition); }
	uint64_t GetStreamSize();
	const char* GetFileName() const { return mPath.c_str(); }

	void ReadData(char* inDest, size_t inSize);

private:
	std::string mPath;
	std::ifstream mStream;
};

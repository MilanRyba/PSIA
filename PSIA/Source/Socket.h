#pragma once
#include <SDKDDKVer.h>

#include <winsock2.h>
#include "ws2tcpip.h"
#include <tchar.h>
#include <string>

struct Packet;
struct AcknowledgePacket;

class Socket
{
public:
	Socket(const std::string inName) : mName(inName) {}
	~Socket() { closesocket(mSocket); }

	// Initialize the socket. Returns false if initialization failed, true otherwise
	bool Initialize(int inLocalPort, uint32_t inMillis);

	void SendPacket(const Packet& inPacket);
	void SendAcknowledgePacket(const AcknowledgePacket& inPacket);
	void SendHash(const std::array<uint8_t, 64>& hashData);

	void RecievePacket(Packet& outPacket);
	void RecieveAcknowledgePacket(AcknowledgePacket& outPacket);

	void FlushAcknowledgements();

	void SetTimeout(uint32_t inMillis)
	{
		DWORD timeout = inMillis;
		setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	}

	// Set the address of the target socket
	void SetAddress(sockaddr_in* inAddress) { mTargetSocket = (sockaddr*)inAddress; }

private:
	SOCKET mSocket = 0;
	sockaddr* mTargetSocket = nullptr;

	sockaddr_in mFrom;
	int mFromSize = 0;

	std::string mName;
	uint32_t mMillis;
	bool mEnableLogging = true;
};
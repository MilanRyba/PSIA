
#include "External/CRC.h"

// #include "stdafx.h"
#include <SDKDDKVer.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"

#include <thread>
#include <iostream>

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define TARGET_PORT 5021
#define LOCAL_PORT 5020

#include "Psia.h"

class Socket
{
public:
	Socket() = default;
	~Socket() { closesocket(mSocket); }

	// Initialize the socket. Returns false if initialization failed, true otherwise
	bool Initialize()
	{
		WSADATA wsaData;
		int success = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (success != 0)
			return false;

		struct sockaddr_in local;
		local.sin_family = AF_INET;
		local.sin_port = htons(LOCAL_PORT);
		local.sin_addr.s_addr = INADDR_ANY;

		mSocket = socket(AF_INET, SOCK_DGRAM, 0);
		int result = bind(mSocket, (sockaddr*)&local, sizeof(local));
		if (result != 0)
			return false;

		return true;
	}


	// Set the address of the target socket
	void SetAddress(sockaddr_in* inAddress) { mTargetSocket = (sockaddr*)inAddress; }

	void SendPacket(Packet& inPacket)
	{
		// Vypoc�tejte CRC-32 hodnotu pro data pred jejich odesl�n�m
		uint32_t crc = CRC::Calculate(inPacket.Payload, inPacket.Size, CRC::CRC_32());

		// Pridejte CRC hodnotu na konec datov�ho bloku
		inPacket.crc = crc;

		// Ode�lete cel� paket, vcetne CRC hodnoty
		sendto(mSocket, (const char*)&inPacket, sizeof(inPacket), 0, mTargetSocket, sizeof(sockaddr_in));
	}


private:
	SOCKET mSocket = 0;
	sockaddr* mTargetSocket = nullptr;
};

int main()
{
	// Name 'socket' is taken by a function...
	Socket s;
	if (!s.Initialize())
		FatalError("[Socket] Binding error!\n");

	const char* file_name = "Resources/BRDF_LUT.png";
	{
		FileStreamReader stream(file_name);
		if (!stream.IsGood())
			FatalError("[FileStreamReader] Failed to open file '%s'\n", file_name);

		using namespace std::chrono_literals;

		// Sleep for 20ms to make sure the receiver is ready
		std::this_thread::sleep_for(20ms);

		// Set an optional pointer to a sockaddr structure that contains the address of the target socket.
		sockaddr_in addrDest;
		addrDest.sin_family = AF_INET;
		addrDest.sin_port = htons(TARGET_PORT);
		InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
		s.SetAddress(&addrDest);

		// Determine the number of packets (of 1KB size) to send. The last packet will be sent separately
		uint64_t size = stream.GetStreamSize();
		int num_packets = (int)size / MAX_PAYLOAD_SIZE;
		int last_packet_size = size % MAX_PAYLOAD_SIZE;

		std::cout << "Size of the payload: " << BytesToString(size) << std::endl;
		std::cout << "Number of packets to send: " << num_packets + 1 << std::endl;
		std::cout << "Size of the last packet: " << BytesToString(last_packet_size) << std::endl;

		Packet packet;
		packet.Type = PacketType::None;
		packet.Size = MAX_PAYLOAD_SIZE;
		for (int i = 0; i < num_packets; i++)
		{
			stream.ReadData((char*)packet.Payload, packet.Size);
			s.SendPacket(packet);
			std::this_thread::sleep_for(10ms);
		}

		// Send the last packet manually
		packet.Type = PacketType::End;
		packet.Size = last_packet_size;
		stream.ReadData((char*)packet.Payload, packet.Size);
		s.SendPacket(packet);
	}

	std::cin.get();
}
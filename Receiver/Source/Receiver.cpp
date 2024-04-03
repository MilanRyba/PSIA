// #include "stdafx.h"
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>

#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define TARGET_PORT 5020
#define LOCAL_PORT 5021

#include "Psia.h"

void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int main()
{
	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;

	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0)
	{
		printf("Binding error!\n");
		std::cin.get();
		return 1;
	}

	{
		FileStreamWriter writer("Resources/brdf.png");

		Packet packet;
		packet.Type = PacketType::None;
		while (packet.Type != PacketType::End)
		{
			if (recvfrom(socketS, (char*)&packet, sizeof(packet), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR)
			{
				printf("Socket error!\n");
				getchar();
				return 1;
			}

			writer.WritePacket(packet);

			static uint32_t num_packets = 1;
			printf("Recieved packet #%i\n", num_packets);
			num_packets++;
		}
	}

	closesocket(socketS);
	std::cin.get();
}

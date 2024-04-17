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
#include "External/CRC.h"

#include <random>

static uint32_t CalculateCRC(uint32_t inSeed)
{
	std::default_random_engine random(inSeed);
	std::uniform_int_distribution<uint32_t> size(0, 1);
	return size(random);
}

static void PollAcknowledgements(Socket& inSocket, Packet& inPacket)
{
	using namespace std::chrono_literals;

	AcknowledgePacket ack;
	inSocket.RecieveAcknowledgePacket(ack);

	std::cout << "  Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;
	
	// TODO: If crc is bad, send again

	while (ack.Acknowledgement == Acknowledgement::BadCRC || ack.Acknowledgement == Acknowledgement::Unknown)
	{
		std::cout << "    Sending again #" << inPacket.ID << "\n";

		// CRC...
		// 
		// Do we need to calculate crc again
		inPacket.CalculateCRC();
		std::cout << "    CRC = " << inPacket.CRC << std::endl;
		
		std::this_thread::sleep_for(5ms);

		if (ack.Acknowledgement == Acknowledgement::Unknown)
		{
			// Acknowledgement::Unknown means that we waited for too long and didn't receive an acknowledgement. So we send the same packet
			// however when we look for the acknowledgement we get the one that we wait
			inSocket.FlushAcknowledgements();
		}

		inSocket.SendPacket(inPacket);
		inSocket.RecieveAcknowledgePacket(ack);

		std::cout << "    Acknowledgement = " << AcknowledgementToString(ack.Acknowledgement) << std::endl;

		// TODO: If we send the same packet like 10 times, terminate
	}
	std::cout << "  Exited acknowledgement polling\n";
}

int main()
{
	// Name 'socket' is taken by a function...
	Socket s = Socket("Sender");
	if (!s.Initialize(LOCAL_PORT, 3000))
		FatalError("[Socket] Binding error!\n");

	const char* file_name = "Resources/photo3.jpg";
	{
		FileStreamReader stream(file_name);
		if (!stream.IsGood())
			FatalError("[FileStreamReader] Failed to open file '%s'\n", file_name);

		using namespace std::chrono_literals;

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
		packet.ID = 0;
		packet.Size = MAX_PAYLOAD_SIZE;
		for (int i = 0; i < num_packets; i++)
		{
			// Read data into the packet
			stream.ReadData((char*)packet.Payload, packet.Size);
			packet.CalculateCRC();

			// Wait for the receiver to be ready
			std::cout << "---------------------------\n";
			std::cout << "Sending packet #" << packet.ID << std::endl;
			std::cout << "CRC = " << packet.CRC << std::endl;
			// std::this_thread::sleep_for(5ms);
			std::this_thread::sleep_for(5ms);
			s.FlushAcknowledgements();
			s.SendPacket(packet);

			PollAcknowledgements(s, packet);
			packet.ID++;
		}

		// Send the last packet manually
		packet.Type = PacketType::End;
		packet.Size = last_packet_size;
		stream.ReadData((char*)packet.Payload, packet.Size);
		packet.CalculateCRC();
		s.SendPacket(packet);
	}

	std::cin.get();
}

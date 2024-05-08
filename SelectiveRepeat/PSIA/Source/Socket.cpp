#include "Socket.h"
#include "stdio.h"
#include "Packet.h"

bool Socket::Initialize(int inLocalPort, uint32_t inMillis)
{
	WSADATA wsaData;
	int success = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (success != 0)
		return false;

	struct sockaddr_in local;

	mFromSize = sizeof(mFrom);
	local.sin_family = AF_INET;
	local.sin_port = htons(inLocalPort);
	local.sin_addr.s_addr = INADDR_ANY;

	mSocket = socket(AF_INET, SOCK_DGRAM, 0);
	int result = bind(mSocket, (sockaddr*)&local, sizeof(local));
	if (result != 0)
		return false;

	mMillis = inMillis;
	SetTimeout(mMillis);

	return true;
}

// https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recvfrom
static void PrintErrorMessage()
{
	int error = WSAGetLastError();
	switch (error)
	{
	case WSANOTINITIALISED:
		printf("Either the application has not called WSAStartup, or WSAStartup failed (WSANOTINITIALISED).\n");
		break;
	case WSAENETDOWN:
		printf("A socket operation encountered a dead network (WSAENETDOWN).\n");
		break;
	case WSAEFAULT:
		printf("The system detected an invalid pointer address in attempting to use a pointer argument in a call (WSAEFAULT).\n");
		break;
	case WSAEINTR:
		printf("A blocking operation was interrupted by a call to WSACancelBlockingCall (WSAEINTR).\n");
		break;
	case WSAEINPROGRESS:
		printf("A blocking operation is currently executing (WSAEINPROGRESS).\n");
		break;
	case WSAEINVAL:
		printf("An invalid argument was supplied (WSAEINVAL).\n");
		break;
	case WSAEISCONN:
		printf("A connect request was made on an already connected socket (WSAEISCONN).\n");
		break;
	case WSAENETRESET:
		printf("The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress (WSAENETRESET).\n");
		break;
	case WSAENOTSOCK:
		printf("An operation was attempted on something that is not a socket (WSAENOTSOCK).\n");
		break;
	case WSAEOPNOTSUPP:
		printf("The attempted operation is not supported for the type of object referenced (WSAEOPNOTSUPP).\n");
		break;
	case WSAESHUTDOWN:
		printf("A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call (WSAESHUTDOWN).\n");
		break;
	case WSAEWOULDBLOCK:
		printf("A non-blocking socket operation could not be completed immediately (WSAEWOULDBLOCK).\n");
		break;
	case WSAEMSGSIZE:
		printf("A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself (WSAEMSGSIZE).\n");
		break;
	case WSAETIMEDOUT:
		printf("A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond (WSAETIMEDOUT).\n");
		break;
	case WSAECONNRESET:
		printf("An existing connection was forcibly closed by the remote host (WSAECONNRESET).\n");
		break;
	}
}

void Socket::SendPacket(const Packet& inPacket)
{
	int ret = sendto(mSocket, (const char*)&inPacket, sizeof(inPacket), 0, mTargetSocket, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
		printf("[%s][SendPacket] Socket Error\n", mName.c_str());
		PrintErrorMessage();
	}
}

void Socket::SendAcknowledgementPacket(const AcknowledgementPacket& inPacket)
{
	int ret = sendto(mSocket, (const char*)&inPacket, sizeof(inPacket), 0, mTargetSocket, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
		printf("[%s][SendAcknowledgementPacket] Socket Error\n", mName.c_str());
		PrintErrorMessage();
	}
}

void Socket::ReceivePacket(Packet& outPacket)
{
	int ret = recvfrom(mSocket, (char*)&outPacket, sizeof(outPacket), 0, (sockaddr*)&mFrom, &mFromSize);
	if (ret == SOCKET_ERROR && mEnableLogging)
	{
		printf("[%s][ReceivePacket] Socket Error\n", mName.c_str());
		PrintErrorMessage();
	}
}

void Socket::ReceiveAcknowledgementPacket(AcknowledgementPacket& outPacket)
{
	int ret = recvfrom(mSocket, (char*)&outPacket, sizeof(outPacket), 0, (sockaddr*)&mFrom, &mFromSize);
	if (ret == SOCKET_ERROR && mEnableLogging)
	{
		printf("[%s][ReceiveAcknowledgementPacket] Socket Error\n", mName.c_str());
		PrintErrorMessage();
	}
}

void Socket::FlushAcknowledgements()
{
	SetTimeout(1);

	mEnableLogging = false;
	AcknowledgementPacket ack;
	ReceiveAcknowledgementPacket(ack);
	mEnableLogging = true;

	SetTimeout(mMillis);
}

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "traceRoute.h"

#pragma comment(lib, "ws2_32.lib")

WORD GenerateChecksum(WORD *pBuf, int iSize)
{
	DWORD cksum = 0;
	while (iSize > 1)
	{
		cksum += *pBuf++;
		iSize -= sizeof(WORD);
	}
	if (iSize)
	{
		cksum += *(BYTE *)pBuf;
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (WORD)(~cksum);
}

int DecodeIcmpResponse(char *pBuf, int iPacketSize, DecodeResult *stDecodeResult)
{
	IpHeader *pIpHdr = (IpHeader *)pBuf;
	WORD usID, usSquNo;
	IcmpHeader *pIcmpHdr;
	int iIpHdrLen = pIpHdr->hdr_len * 4;
	if (iPacketSize < (int)(iIpHdrLen + sizeof(IcmpHeader)))
	{
		return 0;
	}
	pIcmpHdr = (IcmpHeader *)(pBuf + iIpHdrLen);
	if (pIcmpHdr->type == ICMP_ECHO_REPLY)
	{
		usID = pIcmpHdr->id;
		usSquNo = pIcmpHdr->seq;
	}
	else if (pIcmpHdr->type == ICMP_TIMEOUT)
	{
		char * pInnerIpHdr = pBuf + iIpHdrLen + sizeof(IcmpHeader);
		int iInnerIpHdrLen = ((IpHeader *) pInnerIpHdr)->hdr_len * 4;
		IcmpHeader *pInnerIcmpHdr = (IcmpHeader *)(pInnerIpHdr +iInnerIpHdrLen);
		usID = pInnerIcmpHdr->id;
		usSquNo = pInnerIcmpHdr->seq;
	}
	else
	{
		return 0;
	}

	if (pIcmpHdr->type == ICMP_ECHO_REPLY ||
		pIcmpHdr->type == ICMP_TIMEOUT)
	{
		stDecodeResult->ip_addr.s_addr = pIpHdr->sourceIP;
		stDecodeResult->round_trip_time = GetTickCount() - stDecodeResult->round_trip_time;
		printf("%6d ms", stDecodeResult->round_trip_time);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	WSADATA wsa;
	unsigned long ulDestIP;
	struct hostent *pHostent;
	SOCKADDR_IN destSockAddr;
	SOCKET sockRaw;
	int iTimeout;
	char IcmpSendBuf[sizeof(IcmpHeader) + DEF_ICMP_DATA_SIZE];
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
	IcmpHeader *pIcmpHeader;
	DecodeResult stDecodeResult;
	int bReachDestHost;
	WORD usSeqNo;
	int iTTL;
	int iMaxHop;

	// Initialize the WinSock2
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("\nFailed to initialize the WinSock2 DLL\n");
		printf("Error code : %d\n", WSAGetLastError());
		return -1;
	}

	// Get the IP address
	ulDestIP = inet_addr(argv[1]);
	if (ulDestIP == INADDR_NONE)
	{
		pHostent = gethostbyname(argv[1]);
		if (pHostent)
		{
			ulDestIP = (*(IN_ADDR *)pHostent->h_addr).s_addr;
			printf("Tracing route to %s[%s]\n", argv[1], inet_ntoa(*(IN_ADDR *)(&ulDestIP)));
		}
		else
		{
			printf("\nCould not resolve the host name %s\n", argv[1]);
			printf("Error code : %d\n", WSAGetLastError());
			WSACleanup();
			return -1;
		}
	}
	else
	{
		printf("Tracing route to %s\n", argv[1]);
	}

	// Set the address of destination
	ZeroMemory(&destSockAddr, sizeof(SOCKADDR_IN));
	destSockAddr.sin_family = AF_INET;
	destSockAddr.sin_addr.s_addr = ulDestIP;

	// Create the raw socket with ICMP
	sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sockRaw == INVALID_SOCKET)
	{
		printf("\nFailed to create a raw socket\n");
		printf("Error code : %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	iTimeout = DEF_ICMP_TIMEOUT;
	if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char *)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
	{
		printf("\nFailed to set recv timeout\n");
		printf("Error code : %d\n", WSAGetLastError());
		closesocket(sockRaw);
		WSACleanup();
		return -1;
	}

	// Initialize send buffer and recv buffer
	memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));
	memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));

	// set the ICMP package
	pIcmpHeader = (IcmpHeader *)IcmpSendBuf;
	pIcmpHeader->type = ICMP_ECHO_REQUEST;
	pIcmpHeader->code = 0;
	pIcmpHeader->id = (WORD)GetCurrentProcessId();
	memset(IcmpSendBuf + sizeof(IcmpHeader), 'E', DEF_ICMP_DATA_SIZE);

	// Tracing
	bReachDestHost = 0;
	usSeqNo = 0;
	iTTL = 1;
	iMaxHop = DEF_MAX_HOP;
	while((bReachDestHost == 0) && (iMaxHop > 0))
	{
		SOCKADDR_IN from;
		int iFromLen;
		int iReadDataLen;

		setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char *)&iTTL, sizeof(iTTL));
		printf("%3d", iTTL);

		pIcmpHeader->cksum = 0;
		pIcmpHeader->seq = htons(usSeqNo++);
		pIcmpHeader->cksum = GenerateChecksum((WORD *)pIcmpHeader, sizeof(IcmpHeader) + DEF_ICMP_DATA_SIZE);

		stDecodeResult.seq_no = pIcmpHeader->seq;
		stDecodeResult.round_trip_time = GetTickCount();

		if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (SOCKADDR *)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEHOSTUNREACH)
			{
				printf("\tDestination host unreachable.\n");
			}
			closesocket(sockRaw);
			WSACleanup();
			return 0;
		}

		while(1)
		{
			iFromLen = sizeof(from);
			iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (SOCKADDR *)&from, &iFromLen);
			if (iReadDataLen != SOCKET_ERROR)
			{
				if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, &stDecodeResult))
				{
					if (stDecodeResult.ip_addr.s_addr == destSockAddr.sin_addr.s_addr)
					{
						bReachDestHost = 1;
					}
					printf("\t%s\n", inet_ntoa(stDecodeResult.ip_addr));
					break;
				}
			}
			else if (WSAGetLastError() == WSAETIMEDOUT)
			{
				printf("        *\tRequest timed out.\n");
				break;
			}
			else
			{
				printf("\nFailed to call recvfrom\n");
				printf("Error code : %d\n", WSAGetLastError());
				closesocket(sockRaw);
				WSACleanup();
				return -1;
			}
		}
		iTTL++;
	}
	printf("\nTrace complete.\n");
	closesocket(sockRaw);
	WSACleanup();
	return 0;
}
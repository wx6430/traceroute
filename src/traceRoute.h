#ifndef _TRACEROUTE_H_
#define _TRACEROUTE_H_

#pragma pack(1)

typedef struct IpHeader
{
	BYTE hdr_len :4;
	BYTE version :4;
	BYTE tos;
	WORD total_len;
	WORD identifier;
	WORD frag_and_flags;
	BYTE ttl;
	BYTE protocol;
	WORD checksum;
	DWORD sourceIP;
	DWORD destIP;
} IpHeader;

typedef struct IcmpHeader
{
	BYTE type;
	BYTE code;
	WORD cksum;
	WORD id;
	WORD seq;
}IcmpHeader;

typedef struct DecodeResult
{
	WORD seq_no;
	DWORD round_trip_time;
	IN_ADDR ip_addr;
} DecodeResult;

#pragma pack()
#define DEF_ICMP_DATA_SIZE 32
#define MAX_ICMP_PACKET_SIZE 1024

const BYTE ICMP_ECHO_REQUEST = 8; //请求回显
const BYTE ICMP_ECHO_REPLY  = 0; //回显应答
const BYTE ICMP_TIMEOUT   = 11; //传输超时
const DWORD DEF_ICMP_TIMEOUT = 3000; //默认超时时间，单位ms
const int DEF_MAX_HOP = 30;    //最大跳站数

#endif // _TRACEROUTE_H_
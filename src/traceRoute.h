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

const BYTE ICMP_ECHO_REQUEST = 8; //�������
const BYTE ICMP_ECHO_REPLY  = 0; //����Ӧ��
const BYTE ICMP_TIMEOUT   = 11; //���䳬ʱ
const DWORD DEF_ICMP_TIMEOUT = 3000; //Ĭ�ϳ�ʱʱ�䣬��λms
const int DEF_MAX_HOP = 30;    //�����վ��

#endif // _TRACEROUTE_H_
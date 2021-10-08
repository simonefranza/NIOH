#pragma once

#include <stdint.h>
#include <net/if_arp.h>
#include <pthread.h>
#include "machdr.h"
#include "wManager.h"
#include "nw_header.h"

#define MAC_COLON_LEN 17
//#define DEFAULT_IF "wlp3s0"
#define DEFAULT_IF "wlan0" //Wlan used to connect
//#define PLUG_IF "wlxdc4ef4086b85"
//#define VIRT_NET "wlxdc4ef4086b85"
#define PLUG_IF "ni0h"
#define VIRT_NET "ni0h"

typedef struct mapping
{
  struct in_addr* ip_;
  char* ipStr_;
  char* hostname_;
  unsigned char MAC_[ETH_ALEN];
} arpMapping;

typedef struct arp_resp_info_
{
  arpMapping** mapping;
  int selected;
  int size;
  pthread_mutex_t lock;
} arp_resp_info;

typedef struct recvStr
{
  int connectionFd_;
  arp_resp_info* resp_info;
  winStruct* wins_;
} receiverStr;

typedef struct sender_pck_ 
{
  int connectionFd_;
  winStruct* wins_;
} sender_pck;

typedef struct deauth_pck_
{
  int connectionFd_;
  winStruct* wins_;
  char* dmac;
  char* bssid;

} deauth_pck;

typedef struct ptids_
{
  pthread_t wm;
  pthread_t arp_sender;
  pthread_t arp_receiver;
} ptids;

char* getSMac();
char* getBroadcastMac();
int mac2UInt8Arr(const char* mac, uint8_t dest[ETH_ALEN]);
int printEthType(winStruct* wins, struct ether_header* hdr);
int printArpCode(winStruct* wins, struct arphdr* hdr);
void addArpMapping(winStruct* wins, struct arpCont* arpContResp, arp_resp_info* resp_info);
int updateAddr(struct in_addr* currAddr, uint8_t* startAddr, uint8_t* endAddr);
void* sendArpRequest(void* param);
void* deauthAttack(void* param);
void* recvMessage(void* param);
int find(winStruct* wins, struct arpCont* arpContResp, arp_resp_info* resp_info);
void* wlanScan(void* param);

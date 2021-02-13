#pragma once

#include <net/ethernet.h>
#include <arpa/inet.h>

struct arpCont
{
  unsigned char sender_mac[ETH_ALEN];
  struct in_addr sender_ip;
  unsigned char target_mac[ETH_ALEN];
  struct in_addr target_ip;
} __attribute__ ((__packed__));

/* from http://www.radiotap.org/ */
struct ieee80211_radiotap_header {
        u_int8_t        it_version;     /* set to 0 */
        u_int8_t        it_pad;
        u_int16_t       it_len;         /* entire length */
        u_int32_t       it_present;     /* fields present */
} __attribute__((__packed__));

/* radiotap fields */
#define TSFT               (1 << 0)
#define FLAGS              (1 << 1)
#define RATE               (1 << 2)
#define CHANNEL            (1 << 3)
#define FHSS               (1 << 4)
#define DBM_ANT_SIG        (1 << 5)
#define DBM_ANT_NOISE      (1 << 6)
#define LOCK_QUALITY       (1 << 7)
#define TX_ATT             (1 << 8)
#define DB_TX_ATT          (1 << 9)
#define DBM_TX_POW         (1 << 10)
#define ANTENNA            (1 << 11)
#define DB_ANT_SIG         (1 << 12)
#define DB_ANT_NOISE       (1 << 13)
#define RX_FLAGS           (1 << 14)
#define TX_FLAGS           (1 << 15)
#define RSSI               (1 << 16) /* suggested, or RTS_RETRIES */
#define DATA_RETRIES       (1 << 17) /* suggested */
#define CHANNEL_PLUS       (1 << 18)
#define MCS_INFO           (1 << 19)
#define A_MPDU_STATUS      (1 << 20)
#define VHT_INFO           (1 << 21)
#define FRAME_TIMESTP      (1 << 22)
#define HE_INFO            (1 << 23)
#define HE_MU_INFO         (1 << 24)
#define HE_MU_OTHER_U      (1 << 25)
#define LEN0_PSDU          (1 << 26)
#define L_SIG              (1 << 27)
#define RT_RESERVED        (1 << 28)
#define RADTAP_NS_NXT      (1 << 29)
#define VENDOR             (1 << 30)
#define EXT                (1 << 31)
/* radiotap fields end */

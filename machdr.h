#include <net/ethernet.h>

struct frame_control_flags 
{
  int non_strict_order    :1;
  int non_protected_frame :1;
  int no_more_data        :1;
  int power_management    :1;
  int not_retransmission  :1;
  int last_or_unframented :1;
  int not_an_exit         :1;
  int not_to_distr_sys    :1;
} __attribute__ ((__packed__));

struct machdr
{
  int version                       :2; /* 0 for 802.11 WiFi */
  int type                          :2;
  int subtype                       :4;
  struct frame_control_flags fcf;
  unsigned char dur_or_ID[ETH_ALEN];
  unsigned char dmac[ETH_ALEN];
  unsigned char smac[ETH_ALEN];
  unsigned char bssid[ETH_ALEN];
  int seq_num                       :12;
  int frag_num                      :4;
} __attribute__ ((__packed__));

/* MAC Header Type */
#define TYPE_MANAGEMENT   0x00
#define TYPE_CONTROL      0x01
#define TYPE_DATA         0x10
#define RESERVED          0x11

/* Subtypes */
#define SUB_ASSOC         0x0000 // Association Request
#define SUB_PROBE         0x0100 // Probe Request
#define SUB_BEACON        0x1000 // Beacon 
#define SUB_DISASSOC      0x1010 // Disassociation
#define SUB_DEAUTH        0x1100 // Deauthentication 

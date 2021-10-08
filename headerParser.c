#include "headerParser.h"

const char* MAC_HEADER_TYPE[] = {"Management", "Control", "Data", "Reserved"};
const char* MAC_HEADER_SUBTYPE_MANAGEMENT[] = {"Association Request", "", "", "", "Probe Request",
                                               "", "", "", "Beacon", "", "Disassociation", 
                                               "Authentication", "Deauthentication", "", "", ""};

void printRadiotapHeader(struct ieee80211_radiotap_header* hdr, winStruct* wins) {
  wprintw(wins->arp_right->win, "Radiotap header\nVersion\t%d\nLength\t%d\n",
      hdr->it_version, hdr->it_len);
  if(hdr->it_present)
    wprintw(wins->arp_right->win, "FLAGS: %hhx\n");
  uint8_t flags_counter = 0;
  if(hdr->it_present & TSFT)
  {
    uint64_t val = *((uint64_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tTSFT\t%d ms\n", val);
    flags_counter += sizeof(uint64_t);
  }
  if(hdr->it_present & FLAGS)
  {
    uint8_t val = *((uint8_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tFLAGS\t%hhx\n", val);
    flags_counter += sizeof(uint8_t);
  }
  if(hdr->it_present & RATE)
  {
    uint8_t val = *((uint8_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tRATE\t%d Kbps\n", val*500);
    flags_counter += sizeof(uint8_t);
  }
  if(hdr->it_present & CHANNEL)
  {
    uint16_t val = *((uint16_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    flags_counter += sizeof(uint16_t);
    uint16_t flags= *((uint16_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tCHANNEL\t%d MHz, flags %hhx\n", htons(val), flags);
    flags_counter += sizeof(uint16_t);
  }
  if(hdr->it_present & FHSS)
  {
    uint8_t val = *((uint8_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    flags_counter += sizeof(uint8_t);
    uint8_t flags= *((uint8_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tFHSS\tval %d, flags %hhx\n", val, flags);
    flags_counter += sizeof(uint8_t);
  }
  if(hdr->it_present & DBM_ANT_SIG)
  {
    int8_t val = *((uint8_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tANTENNA SIGNAL \t%d dBm\n", val);
    flags_counter += sizeof(uint8_t);
  }
  if(hdr->it_present & DBM_ANT_NOISE)
  {
    int8_t val = *((uint8_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tANTENNA NOISE \t%d dBm\n", val);
    flags_counter += sizeof(uint8_t);
  }
  if(hdr->it_present & LOCK_QUALITY)
  {
    uint16_t val = *((uint16_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tLOCK QUALITY\t%d\n", htons(val));
    flags_counter += sizeof(uint16_t);
  }
  if(hdr->it_present & TX_ATT)
  {
    uint16_t val = *((uint16_t*)hdr+sizeof(struct ieee80211_radiotap_header) + flags_counter);
    wprintw(wins->arp_right->win, "\tTX ATTENUATION\t%d\n", htons(val));
    flags_counter += sizeof(uint16_t);
  }
  wprintw(wins->arp_right->win, "\n");
  update_panels();
  doupdate();
}

void printMACHeader(struct machdr* hdr, winStruct* wins){
  wprintw(wins->arp_right->win, "MAC header\n\tVERSION\t%d\n", hdr->version);
  wprintw(wins->arp_right->win, "\tTYPE\t%s\n", MAC_HEADER_TYPE[hdr->type]);
  wprintw(wins->arp_right->win, "TYPE %hhx",hdr->type);
  update_panels();
  doupdate();
  switch(hdr->type)
  {
    case TYPE_MANAGEMENT:
      wprintw(wins->arp_right->win, "\tSUBTYPE\t%s\n",MAC_HEADER_SUBTYPE_MANAGEMENT[hdr->subtype]);
      break;
    default:
      wprintw(wins->arp_right->win, "\tSUBTYPE\tNot implemented\n");
  }
  wprintw(wins->arp_right->win, "\tReceiver\t");
  for(int i = 0; i <6; i++)
  {
    wprintw(wins->arp_right->win, "%x:", hdr->dmac[i]);
  }
  wprintw(wins->arp_right->win, "\n");
  wprintw(wins->arp_right->win, "\tDestination\t");
  for(int i = 0; i <6; i++)
  {
    wprintw(wins->arp_right->win, "%x:", hdr->smac[i]);
  }
  wprintw(wins->arp_right->win, "\n");
  wprintw(wins->arp_right->win, "\tBSSID\t\t");
  for(int i = 0; i <6; i++)
  {
    wprintw(wins->arp_right->win, "%x:", hdr->bssid[i]);
  }
  wprintw(wins->arp_right->win, "\n");
  if(hdr->type == TYPE_MANAGEMENT && hdr->subtype == SUB_BEACON)
  {
    struct beacon_frame* beac = hdr+sizeof(struct machdr);
    wprintw(wins->arp_right->win, "Timestamp\t%d",htons(beac->timestamp));
    wprintw(wins->arp_right->win, "Beacon Interval\t%d",htons(beac->interval));
    wprintw(wins->arp_right->win, "Capability\t%d",htons(beac->capability));
    wprintw(wins->arp_right->win, "SSID ID\t%d",beac->ssid_id);
    wprintw(wins->arp_right->win, "SSID Len\t%d",beac->ssid_len);
    char* ssid_name = calloc(sizeof(char), beac->ssid_len+1);
    memcpy(ssid_name, beac+sizeof(struct beacon_frame), beac->ssid_len);
    wprintw(wins->arp_right->win, "SSID\t%d",ssid_name);

  }
  wprintw(wins->arp_right->win, "\n");
  update_panels();
  doupdate();

}

#pragma once

#include "nw_header.h"
#include "machdr.h"
#include "wManager.h"

void printRadiotapHeader(struct ieee80211_radiotap_header* hdr, winStruct* wins);
void printMACHeader(struct machdr* hdr, winStruct* wins);

#include "machdr.h"

uint32_t getCRC32(char* mac_hdr, uint64_t hdr_len, char* data, uint64_t data_len)
{
  uint32_t crc32 = 0xFFFFFFFF;
  char* buf = (char*)calloc(hdr_len + data_len, sizeof(char));
  memcpy(buf, mac_hdr, hdr_len);
  memcpy(buf + hdr_len, data, data_len);


  for(int i = 0; i < hdr_len + data_len; i++)
  {
    uint64_t n_lookup = (crc32 ^ buf[i]) & 0xFF;
    crc32 = (crc32 >> 8) ^ crcTable[n_lookup];
  }
  crc32 = crc32 ^ 0xFFFFFFFF;
  free(buf);
  buf = 0;

  return crc32;
}

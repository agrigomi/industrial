#ifndef __URL_CODE__
#define __URL_CODE__

#define URL_DECODE_ERR 65535

size_t UrlDecode(const char* src, char* dest, size_t dstlen);
size_t UrlEncode(const char* src, char* dst, size_t dlen);

#endif

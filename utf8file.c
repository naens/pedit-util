#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "pedit.h"

uint32_t read_utf8_b1(FILE *f, uint8_t byte1);

uint32_t read_utf8(FILE *f)
{
  uint8_t byte1;
  if (fread(&byte1, 1, 1, f) == 1)
    return read_utf8_b1(f, byte1);
  else
    return -1;
}

uint32_t read_utf8_b1(FILE *f, uint8_t byte1)
{
  if (byte1 < 0x80)
    return byte1;

  uint32_t m = 0x80;
  uint32_t res = byte1 & ~m;
  m >>= 1;
  while (res & m)
  {
    uint8_t byte;
    if (fread(&byte, 1, 1, f) != 1)
      return -1;
    res = ((res & ~m) << 6) | (byte & 0x3f);
    m <<= 5;
  }
  return res;
}

/* read one utf-8 character in escaped or raw form */
uint32_t read_char(FILE *f)
{
  int ch;
  if (feof(f))
    return 0;
  ch = fgetc(f);

  if (ch == 0 || ch == '\n' || ch == -1)
    return 0;
  else if (ch != '\\')
    return read_utf8_b1(f, ch);
  if (feof(f))
    return -1;

  ch=fgetc(f);
  if (ch != 'x')
    return -1;
  if (feof(f))
    return -1;

  char line[256];
  int i = 0;
  ch = fgetc(f);
  while (ch >= '0' && ch <= '9'
        || ch >= 'a' && ch <= 'f'
        || ch >= 'A' && ch <= 'F') {
    line[i] = ch;
    i++;
    if (feof(f))
      break;
    ch = fgetc(f);
  }
  line[i] = 0;

  if (i == 0)
    return -1;

  ungetc(ch, f);
  return (uint32_t) strtol(line, NULL, 16);
}

int copyutf8char(char *dest, char *src)
{
  int char_length = utf8chrlen(src);
  if (char_length == -1)
  {
    fprintf(stderr, "utf8 conversion error: %02X\n", (uint8_t) src[0]);
    exit(-1);
  }
  memcpy(dest, src, char_length);
  dest[char_length] = 0;
  return char_length;
}

int getutf8pos(char *str, int pos)
{
  int i = 0;
  int j = 0;
  while (i < pos && str[j] != 0)
  {
    int chlen = utf8chrlen(&str[j]);
    if (chlen == -1)
    {
      fprintf(stderr, "getutf8pos: bad string\n");
      exit(-1);
    }
    j += chlen;
    i++;
  }
  if (i == pos)
    return j;
  else
    return -1; /* error */
}

int utf8writebom(FILE *f)
{
  if (fputc(0xEF, f) == EOF)
    return -1;
  if (fputc(0xBB, f) == EOF)
    return -1;
  if (fputc(0xBF, f) == EOF)
    return -1;
  return 0;
}

int utf8skipbom(FILE *f)
{
  int byte = fgetc(f);
  if (byte == -1)
    return -1;
  if (byte != 0xef)
  {
    ungetc(byte, f);
    return 0;
  }
  byte = fgetc(f);
  if (byte == -1 || byte != 0xbb)
    return -1;
  byte = fgetc(f);
  if (byte == -1 || byte != 0xbf)
    return -1;
  return 0;
}

void write_utf8(FILE *f, uint32_t chr)
{
  int sz;
  uint8_t buf[6];
  ch_ucs32_to_utf8(chr, &sz, buf);
  fwrite(buf, sz, 1, f);
}


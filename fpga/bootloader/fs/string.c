#include <string.h>
#include <stdint.h>
#include <ctype.h>

void* memcpy(void* dest, const void* src, size_t len)
{
  if ((((uintptr_t)dest | (uintptr_t)src | len) & (sizeof(uintptr_t)-1)) == 0) {
    const uintptr_t* s = src;
    uintptr_t *d = dest;
    while (d < (uintptr_t*)(dest + len))
      *d++ = *s++;
  } else {
    const char* s = src;
    char *d = dest;
    while (d < (char*)(dest + len))
      *d++ = *s++;
  }
  return dest;
}

void* memset(void* dest, int byte, size_t len)
{
  if ((((uintptr_t)dest | len) & (sizeof(uintptr_t)-1)) == 0) {
    uintptr_t word = byte & 0xFF;
    word |= word << 8;
    word |= word << 16;
    word |= word << 16 << 16;

    uintptr_t *d = dest;
    while (d < (uintptr_t*)(dest + len))
      *d++ = word;
  } else {
    char *d = dest;
    while (d < (char*)(dest + len))
      *d++ = byte;
  }
  return dest;
}

size_t strlen(const char *s)
{
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

int strcmp(const char* s1, const char* s2)
{
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
  } while (c1 != 0 && c1 == c2);

  return c1 - c2;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
    n--;
  } while (c1 != 0 && c1 == c2 && n > 0);
  
  return c1 -c2;
}

char* strcpy(char* dest, const char* src)
{
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

char* strncpy(char* dest, const char* src, size_t n)
{
  char* d = dest ;
  const char* s = src;
  for (;(s < src + n )&&(*d = *s); d++, s++)
    ;
  return dest;
}

char* strchr( const char* s, int c)
{
  while(*s && (*s != (char)c))
    s++;
  
  if((c != 0) && (*s == 0))
    return NULL;
  else 
    return (char*)s;
}

char* strrchr(const char* s, int c)
{
  char * p = (char* ) s + strlen(s);
  while (p >= s && (*p != (char) c))
    p--;
  
  if (p < s)
    return NULL;
  else 
    return (char*) p;
}

char * strtok(char * s, const char* delim)
{
  static char * s_end = NULL;
  static char * cur_ptr = NULL;

  if(s) {
    s_end = s + strlen(s);
    cur_ptr = s;
  }

  if(!s_end)
    return NULL; // err: no initial s;

  if(cur_ptr >= s_end)
    return NULL;

  char* end_ptr = cur_ptr;
  unsigned int delim_len = strlen(delim);
  
  int in_token = 0;
  while(end_ptr < s_end){
    char ch = * end_ptr;
    int i = 0;
    for(i = 0; i< delim_len;i++){
      if(ch == delim[i]) break;
    }
    if(i < delim_len){
      if(in_token){
        // end of token
        * end_ptr = '\0';
        break;
      }
      else {
        // leading delim chars
      }
    }
    else {
      if (in_token) {
        // in token
      }
      else {
        // beginning of token
        cur_ptr = end_ptr;
        in_token = 1;
      }
    }
    end_ptr ++;
  }

  char * ret_val = NULL;

  if(in_token) ret_val = cur_ptr;
  cur_ptr = end_ptr + 1;

  return ret_val;

}

long atol(const char* str)
{
  long res = 0;
  int sign = 0;

  while (*str == ' ')
    str++;

  if (*str == '-' || *str == '+') {
    sign = *str == '-';
    str++;
  }

  while (*str) {
    res *= 10;
    res += *str++ - '0';
  }

  return sign ? -res : res;
}

#include <stdarg.h>
int xy_strcpylen(char *dst,const char *src); 
int xy_strncpylen(char *dst,const char *src,int maxlen); 
int xy_prntstrncpylen(char *dst,const char *src,int maxlen);

int xy_strlen(const char *str); 
int xy_s32toa(char *buf,int n);
int xy_strniseq(const char *str1,const char *str2,int n);
int xy_striseq(const char *str1,const char *str2);
char * xy_strstr(const char *haystack,const char *needle);
int xy_strncat(char *dest,int maxlen,...);

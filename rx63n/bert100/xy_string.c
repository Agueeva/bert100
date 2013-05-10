/**
 *************************************************************
 * String library for XY-0815 Webserv
 * (C) 2002 Jochen Karrer
 *************************************************************
 */
#include "xy_string.h"
#include <stdlib.h>
#include <ctype.h>
#include "events.h"
/*
 * ----------------------------------
 * strcpy with length as return value
 * ----------------------------------
 */
int
xy_strcpylen(char *dst,const char *src) {
	int i=0;
	while((*dst++=*src++)) {
		i++;
	}
	EV_Yield();
	return i;
}

int
xy_strncpylen(char *dst,const char *src,int maxlen) {
	int i=0;
	do {
		*dst++ = *src++;
		i++;
	} while((i < maxlen) && *src);
	EV_Yield();
	return i;
}

#if 0
int
xy_prntstrncpylen(char *dst,const char *src,int maxlen) 
{
	int i=0,j=0;
	while((j < maxlen) && (*dst=*src++)) {
		if(isprint(*dst)) {
			dst++;
			i++;
		}
		j++;
	}
	return i;
}
#endif
/*
 * -----------------------------
 * Standard strlen
 * -----------------------------
 */
int
xy_strlen(const char *str) {
	const char *start=str;
	while(*str){
		str++;
	}
	return str-start; 	
}

int
xy_strniseq(const char *str1,const char *str2,int n) {
        int i;
        for(i=0;i<n;i++) {
                if(str1[i]!=str2[i]) {
                        return 0;
                }
                if(!str1[i])
                        return 1;
        }
        return 1;
}
int
xy_striseq(const char *str1,const char *str2) {
        for(;(*str1==*str2)&&*str1;str1++,str2++) {

        }
	if(*str1 || *str2 )
		return 0;
	else
        	return 1;
}

/*
 * --------------------------------------------
 * Convert an 32 Bit Integer to a string
 * --------------------------------------------
 */
int
xy_s32toa(char *buf,int n)
{
        int cnt=0;
        char *revstart=buf;
        if(n<0) {
                cnt=1;
                n=-n; *buf++='-'; revstart++;
        }
        *buf++=0;
	do {
		cnt++;
		*(buf++)=n%10+'0';
		n=n/10;
	} while(n);
        while(buf-->revstart) {
                char c=*buf; *buf=*revstart; *revstart++=c;
        }
        return cnt;
}

char *
xy_strstr(const char *haystack,const char *needle) 
{
	const char *n=needle,*h;
	while(*haystack) {
		h=haystack;
		while(*n && *h) {
			if(*n != *h) 
				break;
			n++;h++;
		}
		if(*n==0) 
			break;
		haystack++;
		n=needle;
	}
	if(*n==0)
		return (char*)haystack;
	return NULL;
} 

#if 0
int
xy_strncat(char *dest,int maxlen,...) {
	int count = 0;
	va_list ap;
	va_start (ap, maxlen);
	char *str;
	while((str = va_arg(ap, char *)) != NULL) {
		while(*str && (count < (maxlen - 1))) {
			dest[count++] = *str++;
		}	
	}
	dest[count] = 0;
	va_end(ap);
	return count;
}
#endif


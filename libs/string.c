#include "string.h"

inline void memcpy(uint8_t *dest,const uint8_t* src,uint32_t len)
{
	uint8_t* pdest=dest;
	const uint8_t*	psrc=src;
	for(;len!=0;len--)
	{
		*pdest=*psrc;
		pdest++;
		psrc++;
	}
}


inline void memset(void* dest,uint8_t val,uint32_t len)
{
	uint8_t* pdest=(uint8_t*)dest;
	uint32_t i=0;
	while(i<len)
	{
		*pdest=val;
		pdest++;
		i++;
		
	}
}


void bzero(void *dest, uint32_t len)
{

	memset(dest,0,len);
}

int strcmp(const char *str1, const char *str2)
{
	const char* pstr1=str1;
	const char* pstr2=str2;
	while(*pstr1!='\0'&&*pstr2!='\0')
	{
		if(*pstr1==*pstr2)
		{
			pstr1++;
			pstr2++;
		}
		else if(*pstr1>*pstr2)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	if(*pstr1=='\0'&&*pstr2=='\0')
	{
		return 0;
	}
	else if(*pstr2=='\0')
	{
		return 1;
	}
	else
	{
		return -1;
	}
}


char *strcpy(char *dest, const char *src)
{
	char* pdest=dest;
	const char* psrc=src;
	while(*src!='\0')
	{
		*pdest=*psrc;
		psrc++;
		pdest++;
	}
	*pdest='\0';
	return dest;
}


char *strcat(char *dest, const char *src)
{
	char* pdest=dest;
	const char* psrc=src;
	while(*pdest!='\0')
	{
		pdest++;
	}
	strcpy(pdest,psrc);
	return dest;
}

int strlen(const char *src)
{
	int i=0;
	const char* psrc=src;
	while(*psrc!='\0')
	{
		psrc++;
		i++;
	}
	return i;
}
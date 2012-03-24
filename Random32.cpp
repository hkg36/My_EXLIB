#include "Random32.h"
#include <windows.h>

unsigned long RandomFunc(unsigned long one)
{
	enum{
		a=4096,
		c=150889,
		m=714025
	};
	return (a*one+c)%m;
}
CRandom32::CRandom32(unsigned long seed)
{
	if(seed==0)
	{
		if(now==(unsigned long)-1)
		{
			FILETIME ft;
			::GetSystemTimeAsFileTime(&ft);
			now=ft.dwHighDateTime^ft.dwLowDateTime;
		}
	}
	else
	{
		now=seed;
	}
}
unsigned long CRandom32::Next()
{
	now=RandomFunc(now);
	return now;
}
__declspec( thread ) unsigned long CRandom32::now=(unsigned long)-1;

CRandom32Self::CRandom32Self(unsigned long seed)
{
	if(seed==0)
	{
		FILETIME ft;
		::GetSystemTimeAsFileTime(&ft);
		now=ft.dwHighDateTime^ft.dwLowDateTime;
	}
	else
	{
		now=seed;
	}
}
unsigned long CRandom32Self::Next()
{
	now=RandomFunc(now);
	return now;
}
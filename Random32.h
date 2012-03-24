#pragma once
#include <windows.h>
struct IRandom
{
	virtual unsigned long Next()=0;
};
unsigned long RandomFunc(unsigned long one);
class CRandom32:public IRandom
{
private:
	__declspec( thread ) static unsigned long now;
public:
	CRandom32(unsigned long seed=0);
	unsigned long Next();
};
class CRandom32Self:public IRandom
{
private:
	unsigned long now;
public:
	CRandom32Self(unsigned long seed=0);
	unsigned long Next();
};
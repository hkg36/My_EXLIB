#pragma once
#include <atlbase.h>
#include <atlstr.h>
#include <vector>
using namespace std;
class CBase64
{
public:
	static CAtlStringA Encode(const void *in_, int inlen);
	static size_t Decode(vector<char> &out, const char *in);
};

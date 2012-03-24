#include "Base64.h"

const char base64digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char BAD=-1;

CAtlStringA CBase64::Encode(const void *in_, int inlen)
{
	CAtlStringA out;
	const unsigned char *in = (const unsigned char *)in_;
	for (; inlen >= 3; inlen -= 3) 
	{
		out.AppendChar(base64digits[in[0] >> 2]);
		out.AppendChar(base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)]);
		out.AppendChar(base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)]);
		out.AppendChar(base64digits[in[2] & 0x3f]);
		in += 3;
	}
	if (inlen > 0) 
	{
		unsigned char fragment;
		out.AppendChar(base64digits[in[0] >> 2]);
		fragment = (in[0] << 4) & 0x30;
		if (inlen > 1)  fragment |= in[1] >> 4;
		out.AppendChar(base64digits[fragment]);
		out.AppendChar((inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c]);
		out.AppendChar('=');
	}
	return out;
}
static const char base64val[] =
{
  BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,  BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,  BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD, 62, BAD,BAD,BAD, 63,  52, 53, 54, 55, 56, 57, 58, 59, 60, 61,BAD,BAD, BAD,BAD,BAD,BAD,  BAD, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,BAD, BAD,BAD,BAD,BAD,  BAD, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,BAD, BAD,BAD,BAD,BAD
};
inline char DECODE64(char in)
{
	if(in>=_countof(base64val)) return BAD;
	return base64val[in];
}
size_t CBase64::Decode(vector<char> &out, const char *in)
{
	register unsigned char digit1, digit2, digit3, digit4;
	if (in[0] == '+' && in[1] == ' ') in += 2;
	if (*in == '\r') return(0);
	do
	{
		digit1 = in[0];
		if (DECODE64(digit1) == BAD)  return(-1);
		digit2 = in[1];
		if (DECODE64(digit2) == BAD)  return(-1);
		digit3 = in[2];
		if (digit3 != '=' && DECODE64(digit3) == BAD)  return(-1);
		digit4 = in[3];
		if (digit4 != '=' && DECODE64(digit4) == BAD)  return(-1);
		in += 4;
		out.push_back((DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4));
		if (digit3 != '=')
		{
			out.push_back(((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2));
			if (digit4 != '=') 
			{
				out.push_back(((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4));
			}
		}
	}
	while (*in && *in != '\r' && digit4 != '=');
	return out.size();
}
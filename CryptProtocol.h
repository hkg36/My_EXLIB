#pragma once
#include "zlib.h"
#include "BigInt.h"
#include "cexcrypt.h"
#include "HashTools.h"
#include "ByteStream.h"
#include <winsock2.h>
#include <vector>
using namespace std;

#ifndef USE_STATIC_RSAKEY
#define USE_STATIC_RSAKEY 1
#endif
#if USE_STATIC_RSAKEY
#define RSA_FUNC_STATIC static
#else
#define RSA_FUNC_STATIC
#endif
class CCryptProtocol
{
private:
	RSA_FUNC_STATIC CBigInt E_D,N;
	CBigInt crypted;
	CAES3 aes;
	CSHA1 sha1;
	UCHAR type;
	struct HeadPack
	{
		UINT cryptlen;
		UINT srclen;
		Sha1Value sha1;
		USHORT aeskey[16];
		UCHAR packback[56];
		ULONG headcrc;
	} headpack;
	CComPtr<IMemoryStream> bufferstream;
	z_stream zstr;
	int m_mode;
	UCHAR m_pack_mod;
	int ReInitZstr(int newmode);
	char pubbuf[128];
	vector<WSABUF> wsabufs;
public:
	UCHAR GetPackMode(){return m_pack_mod;}
	enum {NoneMode,PackMode,UnPackMode};
	enum {NORMAL=1,RSAPACK=2};
	CCryptProtocol(void);
	~CCryptProtocol(void);
	CComPtr<IMemoryStream> BodyStream(){return (IMemoryStream*)bufferstream;}
	void Init(int mod);
	void SetPackBack(UCHAR (&buf)[56],UINT len);
	void WriteContent(void* buf,UINT len);
	void WriteEnd();
	WSABUF* BuildPack(int packmod,size_t &wsabufcount);
	bool SavePack(void *buf,UINT &len);
	RSA_FUNC_STATIC void SetRsaKey(CAtlStringA E_D,CAtlStringA N);

	int RecvMode(UCHAR mod);
	int RecvHeadPack(void* buf,int buflen);
	bool RecvMainBody(void* buf,int buflen);
};

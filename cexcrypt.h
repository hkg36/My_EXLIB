#pragma once
#include "rijndael.h"
#define E_NEEDEXPANDMEMORY 0xff003
#define E_ERRORPAKAGE 0xff002
class CAESBase
{
	rijndael aes;
	bool haskey;
public:
	CAESBase();
	LRESULT SetKey(void *key,int keylength);
	LRESULT EndcodeDate(void *data,int &datalength);
	LRESULT DecodeData(void *data,int &datalength);
};

class CAES2Base
{
private:
	UCHAR prekey[16];
	rijndael aes;
	bool haskey;
	int keysize;
	UCHAR startkey[256/8];
	UCHAR nowkey[32];
	void UpdateRunKey(UCHAR *newpart);
	void InitKey();
public:
	CAES2Base();
	LRESULT SetKey(void *key,int keylength);
	LRESULT EndcodeDate(void *data,int &datalength);
	LRESULT DecodeData(void *data,int &datalength);
};

class CAES3
{
	rijndael aes;
	bool haskey;
	ULONGLONG lastblock[2];
public:
	void Init();
	bool SetKey(void *key,int keylength);
	bool EndcodeDate(void *data,int &datalength);
	bool DecodeData(void *data,int &datalength);
	bool EndcodeDate(void *data,void* outdata,int &datalength);
	bool DecodeData(void *data,void* outdata,int &datalength);
};
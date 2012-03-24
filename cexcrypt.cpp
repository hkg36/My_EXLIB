#include <stdlib.h>
#include "cexarray.h"
#include <Windows.h>
#include "cexcrypt.h"

CAESBase::CAESBase():haskey(false){}
LRESULT CAESBase::SetKey(void *key,int keylength)
{
	if(keylength==128/8 || keylength== 192/8 || keylength == 256/8)
	{
		aes.set_key((u1byte*)key,keylength*8);
		haskey=true;
		return S_OK;
	}
	else
		return S_FALSE;
}
LRESULT CAESBase::EndcodeDate(void *data,int &datalength)
{
	if(haskey==false) return S_FALSE;
	int needlength=UPROUND(datalength,16);
	if(needlength!=datalength)
	{
		datalength=needlength;
		return E_NEEDEXPANDMEMORY;
	}
	for(int i=0;i<datalength;i+=16)
	{
		aes.encrypt((u1byte*)data+i);
	}
	return S_OK;
}
LRESULT CAESBase::DecodeData(void *data,int &datalength)
{
	if(haskey==false) return S_FALSE;
	if(datalength%16!=0)
		return E_ERRORPAKAGE;
	for(int i=0;i<datalength;i+=16)
	{
		aes.decrypt((u1byte*)data+i);
	}
	return S_OK;
}

void CAES2Base::UpdateRunKey(UCHAR *newpart)
{
	*(__int64*)(nowkey)^=*(__int64*)newpart;
	*(__int64*)(nowkey+8)^=*(__int64*)(newpart+8);
	*(__int64*)(nowkey+16)^=*(__int64*)newpart;
	*(__int64*)(nowkey+24)^=*(__int64*)(newpart+8);
	aes.set_key((u1byte*)nowkey,256);
}
void CAES2Base::InitKey()
{
	memcpy(nowkey,startkey,32);
	aes.set_key(nowkey,256);
	UCHAR tempprekey[16];
	memcpy(tempprekey,prekey,16);
	aes.encrypt(tempprekey);
	UpdateRunKey(tempprekey);
}
CAES2Base::CAES2Base():haskey(false)
{
}
LRESULT CAES2Base::SetKey(void *key,int keylength)
{
	if(keylength == 256/8)
	{
		memcpy(startkey,key,32);
		//aes.set_key((u1byte*)key,keylength*8);
		*(__int64*)&prekey[0]=*(__int64*)&startkey[0]^*(__int64*)&startkey[8];
		*(__int64*)&prekey[8]=*(__int64*)&startkey[16]^*(__int64*)&startkey[24];

		haskey=true;
		return S_OK;
	}
	else
		return S_FALSE;
}
LRESULT CAES2Base::EndcodeDate(void *data,int &datalength)
{
	if(haskey==false) return S_FALSE;
	int needlength=UPROUND(datalength,16);
	if(needlength!=datalength)
	{
		datalength=needlength;
		return E_NEEDEXPANDMEMORY;
	}
	InitKey();
	for(int i=0;i<datalength;i+=16)
	{
		aes.encrypt((u1byte*)data+i);
		UpdateRunKey((u1byte*)data+i);
	}
	return S_OK;
}
LRESULT CAES2Base::DecodeData(void *data,int &datalength)
{
	if(haskey==false) return S_FALSE;
	if(datalength%16!=0)
		return E_ERRORPAKAGE;
	InitKey();
	UCHAR lastencode[16];
	for(int i=0;i<datalength;i+=16)
	{
		memcpy(lastencode,(u1byte*)data+i,16);
		aes.decrypt((u1byte*)data+i);
		UpdateRunKey(lastencode);
	}
	return S_OK;
}
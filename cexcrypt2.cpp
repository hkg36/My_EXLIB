#include "cexarray.h"
#include "cexcrypt.h"

void CAES3::Init()
{
	lastblock[0]=0x123456789abcdef1;
	lastblock[1]=0x13579bdf2468ace1;
}
bool CAES3::SetKey(void *key,int keylength)
{
	if(keylength==256/8 || keylength==192/8 || keylength==128/8)
	{
		aes.set_key((const u1byte*)key,keylength*8);
		haskey=true;
		Init();
		return true;
	}
	return false;
}
bool CAES3::EndcodeDate(void *data,int &datalength)
{
	return EndcodeDate(data,data,datalength);
}
bool CAES3::DecodeData(void *data,int &datalength)
{
	return DecodeData(data,data,datalength);
}

bool CAES3::EndcodeDate(void *data,void* outdata,int &datalength)
{
	if(haskey==false) return false;
	int needlength=UPROUND(datalength,16);
	if(needlength!=datalength)
	{
		datalength=needlength;
		return false;
	}

	for(int i=0;i<datalength;i+=16)
	{
		ULONGLONG *now1=(ULONGLONG*)((u1byte*)data+i);
		ULONGLONG *now2=(ULONGLONG*)((u1byte*)data+i+8);
		ULONGLONG *now1out=(ULONGLONG*)((u1byte*)outdata+i);
		ULONGLONG *now2out=(ULONGLONG*)((u1byte*)outdata+i+8);

		*now1out=lastblock[0]^*now1;
		*now2out=lastblock[1]^*now2;
		aes.encrypt((u1byte*)now1out,(u1byte*)lastblock);
		*now1out=lastblock[0];
		*now2out=lastblock[1];
	}

	return true;
}
bool CAES3::DecodeData(void *data,void* outdata,int &datalength)
{
	if(haskey==false) return false;
	int needlength=UPROUND(datalength,16);
	if(needlength!=datalength)
	{
		datalength=needlength;
		return false;
	}

	ULONGLONG refblock[2];
	for(int i=0;i<datalength;i+=16)
	{
		ULONGLONG *now1=(ULONGLONG*)((u1byte*)data+i);
		ULONGLONG *now2=(ULONGLONG*)((u1byte*)data+i+8);
		ULONGLONG *now1out=(ULONGLONG*)((u1byte*)outdata+i);
		ULONGLONG *now2out=(ULONGLONG*)((u1byte*)outdata+i+8);

		refblock[0]=lastblock[0];
		refblock[1]=lastblock[1];
		lastblock[0]=*now1;
		lastblock[1]=*now2;
		aes.decrypt((u1byte*)lastblock,(u1byte*)now1);
		*now1out=refblock[0]^*now1;
		*now2out=refblock[1]^*now2;
	}
	return true;
}
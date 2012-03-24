#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include "zutil.h"
#include "zlib.h"
#pragma message("======================>>不要使用GzipCaps.h，改用gzipcap2.h")   
class CGZCap
{
private:
	static const int t_nBufferLength = 1024;
	CComPtr<IStream> out;
	CHAR m_buffer[t_nBufferLength];
	z_stream m_zstream;
	uLong  m_crc;
	bool started;
	int Destroy();
	void Write(LPCSTR buf,ULONG bufsz);
	void WriteLong (uLong x);
public:
	CGZCap(int nLevel=Z_DEFAULT_COMPRESSION,int nStrategy=Z_DEFAULT_STRATEGY);
	bool Init(IStream *output);
	void Reset();
	~CGZCap();
	void Input(LPCSTR buf,int bufsz);
	void Finish();
};

class CUGZCap
{
private:
	static const int t_nBufferLength = 1024;
	CComPtr<IStream> out;
	CHAR m_buffer[t_nBufferLength];
	z_stream m_zstream;
	uLong  m_crc;
	void Destroy();
	void Write(LPCSTR buf,ULONG bufsz);
public:
	CUGZCap();
	bool Init(IStream* output);
	void Reset();
	~CUGZCap();
	bool Input(LPCSTR buff,int bufflen,LPCSTR *tailcheckstart);
	bool TailCheck(LPCSTR tailcheckstart,int size);
};

class CGZHeadSkiper
{
private:
	UINT stat;
	UINT count;
	UCHAR method;
	UCHAR flags;
	bool error;
	UINT mtime;
	UCHAR xflag;
	UCHAR oscode;
	UCHAR headcrc[2];
	CAtlStringA origname;
	CAtlStringA comment;
	bool CheckNext(UCHAR c);
	void SetError(){error=true;};
public:
	UINT MTime(){return mtime;}
	UCHAR OSCode(){return oscode;}
	CAtlStringA OrignName(){return origname;}
	CAtlStringA Comment(){return comment;}
	bool IsError(){return error;}
	void Init();
	CGZHeadSkiper();
	bool Input(LPCSTR buffer,int len,LPCSTR* headend);
};
/* 压缩例子
if(false)
{
	CComPtr<IStream> outstr;
	FileStream::CreateNewFile(L"D:/exeoutput.gz",&outstr);
	CGZCap cap(outstr);
	CAtlFile fread;
	fread.Create(L"D:/sogou_pinyin_42.exe",FILE_READ_DATA,FILE_SHARE_READ,OPEN_EXISTING);
	char buf[396];
	DWORD read;
	while(fread.Read(buf,sizeof(buf),read)==S_OK)
	{
		if(read==0) break;
		cap.Input(buf,read);
	}
	fread.Close();
	cap.Finish();
	return 0;
}
*/
/* 解压例子 （完全流读取模式）
if(true)
{
	bool res;
	CAtlFile fread;
	fread.Create(L"D:/sogou_pinyin_42.exe.gz",FILE_READ_DATA,FILE_SHARE_READ,OPEN_EXISTING);
	char buf[512];
	DWORD read;
	CComPtr<IStream> outstr;
	FileStream::CreateNewFile(L"D:/sogou_pinyin_42_2.exe",&outstr);
	CUGZCap cap(outstr);

	CGZHeadSkiper skiper;
	LPCSTR tail=nullptr;
	LPCSTR skiphead;
	while(fread.Read(buf,3,read)==S_OK)
	{
		if(read==0) return 0;
		if(skiper.Input(buf,read,&skiphead)==false)
		{
			if(skiper.IsError()) return 0;
			break;
		}
	}
	if(cap.Input(skiphead,read-(skiphead-buf),&tail))
	{
		while(fread.Read(buf,sizeof(buf),read)==S_OK)
		{
			if(read==0) break;
			if(!cap.Input(buf,read,&tail))
			{
				break;
			}
		}
	}
	read=read-(tail-buf);
	memcpy(buf,tail,read);
	if(read<8)
	{
		fread.Read(buf+read,16-read,read);
	}
	res=cap.TailCheck(buf,8);
	return 0;
}*/
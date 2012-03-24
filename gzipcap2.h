#pragma once
#include <atlbase.h>
#include "zutil.h"
#include "zlib.h"

class GZBase
{
protected:
	static const int t_nBufferLength = 1024;
	z_stream zstream;
	CHAR m_buffer[t_nBufferLength];
	CComPtr<IStream> out;
	GZBase(CComPtr<IStream> out,int windowBits,int nLevel=Z_DEFAULT_COMPRESSION,int nStrategy=Z_DEFAULT_STRATEGY)
	{
		this->out=out;
		ZeroMemory(&zstream,sizeof(zstream));
		int res=deflateInit2(&zstream,nLevel,Z_DEFLATED,windowBits, DEF_MEM_LEVEL,nStrategy);
		ATLASSERT(res==Z_OK);
	}
public:
	int Reset(CComPtr<IStream> out=nullptr)
	{
		if(out)
			this->out=out;
		int res=deflateReset(&zstream);
		ATLASSERT(res==Z_OK);
		return res;
	}
	int Write(const void* buffer,size_t buflen)
	{
		if(buflen==0) return 0;
		zstream.next_in=(Bytef*)buffer;
		zstream.avail_in=buflen;
		zstream.total_out=0;

		while(zstream.avail_in>0)
		{
			zstream.next_out=(Bytef*)m_buffer;
			zstream.avail_out=t_nBufferLength;
			int resd = deflate(&zstream,Z_FULL_FLUSH);
			ATLASSERT(resd==Z_OK);
			out->Write(m_buffer,(char*)zstream.next_out-m_buffer,nullptr);
		}
		return zstream.total_out;
	}
	void Finish()
	{
		zstream.avail_in=0;
		zstream.next_in=nullptr;
		while(true)
		{
			zstream.next_out=(Bytef*)m_buffer;
			zstream.avail_out=t_nBufferLength;
			int res=deflate(&(zstream), Z_FINISH);
			if(res==Z_OK)
			{
				out->Write(m_buffer,(char*)zstream.next_out-m_buffer,nullptr);
			}
			else if(res==Z_STREAM_END)
			{
				out->Write(m_buffer,(char*)zstream.next_out-m_buffer,nullptr);
				break;
			}
		}
	}
	~GZBase()
	{
		int res=deflateEnd(&zstream);
		ATLASSERT(res==Z_OK);
	}
};
class GZIPCap2:public GZBase
{
public:
	gz_header gzheader;
	GZIPCap2(CComPtr<IStream> out=nullptr,int nLevel=Z_DEFAULT_COMPRESSION,int nStrategy=Z_DEFAULT_STRATEGY):
		GZBase(out,MAX_WBITS|0x10,nLevel,nStrategy)
	{
		ZeroMemory(&gzheader,sizeof(gzheader));
		int res=deflateSetHeader(&zstream,&gzheader);
		ATLASSERT(res==Z_OK);
	}
};
class DeflateCap:public GZBase
{
public:
	DeflateCap(CComPtr<IStream> out=nullptr,int nLevel=Z_DEFAULT_COMPRESSION,int nStrategy=Z_DEFAULT_STRATEGY):
		GZBase(out,-MAX_WBITS,nLevel,nStrategy)
		{
		}
};

class UnGZBase
{
	protected:
	static const int t_nBufferLength = 1024;
	z_stream zstream;
	CHAR m_buffer[t_nBufferLength];
	CComPtr<IStream> out;
	UnGZBase(CComPtr<IStream> out,int windowBits)
	{
		this->out=out;
		ZeroMemory(&zstream,sizeof(zstream));
		int res=inflateInit2(&zstream,windowBits);
		ATLASSERT(res==Z_OK);
	}
public:
	int Reset(CComPtr<IStream> out=nullptr)
	{
		if(out)
			this->out=out;
		int res=inflateReset(&zstream);
		ATLASSERT(res==Z_OK);
		return res;
	}
	int Write(const void* buffer,size_t buflen,size_t *writen)
	{
		if(buflen==0) return true;
		zstream.next_in=(Bytef*)buffer;
		zstream.avail_in=buflen;

		while(zstream.avail_in>0)
		{
			zstream.next_out=(Bytef*)m_buffer;
			zstream.avail_out=t_nBufferLength;
			int resd = inflate(&zstream,Z_FULL_FLUSH);
			if(resd==Z_OK || resd==Z_STREAM_END)
			{
				out->Write(m_buffer,(char*)zstream.next_out-m_buffer,nullptr);
				if(resd==Z_STREAM_END)
				{
					if(writen) *writen=buflen-zstream.avail_in;
					return 0;
				}
			}
			else
			{
				if(writen) *writen=0;
				return -1;
			}
		}
		if(writen) *writen=buflen-zstream.avail_in;
		return 1;
	}
	~UnGZBase()
	{
		int res=inflateEnd(&zstream);
		ATLASSERT(res==Z_OK);
	}
};
class InflateCap:public UnGZBase
{
public:
	InflateCap(CComPtr<IStream> out=nullptr):UnGZBase(out,-MAX_WBITS)
	{
	}
};

class GUNZIPCap2:public UnGZBase
{
public:
	gz_header gzheader;
	GUNZIPCap2(CComPtr<IStream> out=nullptr):UnGZBase(out,MAX_WBITS|0x10)
	{
		ZeroMemory(&gzheader,sizeof(gzheader));
		int res=inflateGetHeader(&zstream,&gzheader);
		ATLASSERT(res==Z_OK);
	}
};
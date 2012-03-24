#include "CryptProtocol.h"
#include "cexarray.h"
#include "Random32.h"

#if USE_STATIC_RSAKEY
CBigInt CCryptProtocol::E_D;
CBigInt CCryptProtocol::N;
#endif
CCryptProtocol::CCryptProtocol(void):m_mode(NoneMode)
{
	static_assert(sizeof(HeadPack)==120,"size of HeadPack must be 120");
	ZeroMemory(&headpack,sizeof(headpack));
	ZeroMemory(&zstr,sizeof(zstr));
	ByteStream::CreateInstanse(&bufferstream);
}

CCryptProtocol::~CCryptProtocol(void)
{
	switch(m_mode)
	{
	case PackMode:
		{
			::deflateEnd(&zstr);
		}break;
	case UnPackMode:
		{
			::inflateEnd(&zstr);
		}break;
	}
}
int CCryptProtocol::ReInitZstr(int newmode)
{
	int res=Z_ERRNO;
	switch(m_mode)
	{
	case NoneMode:
		{
			switch(newmode)
			{
			case PackMode:
				{
					res=deflateInit(&zstr,Z_BEST_COMPRESSION);
				}break;
			case UnPackMode:
				{
					res=inflateInit(&zstr);
				}break;
			}
		}break;
	case PackMode:
		{
			switch(newmode)
			{
			case PackMode:
				{
					res=::deflateReset(&zstr);
				}break;
			case UnPackMode:
				{
					res=::deflateEnd(&zstr);
					ZeroMemory(&zstr,sizeof(zstr));
					res=inflateInit(&zstr);
				}break;
			}
		}break;
	case UnPackMode:
		{
			switch(newmode)
			{
			case PackMode:
				{
					res=::inflateEnd(&zstr);
					ZeroMemory(&zstr,sizeof(zstr));
					res=deflateInit(&zstr,Z_BEST_COMPRESSION);
				}break;
			case UnPackMode:
				{
					res=::inflateReset(&zstr);
				}break;
			}
		}break;
	}
	return res;
}
void CCryptProtocol::SetRsaKey(CAtlStringA E_D_str,CAtlStringA N_str)
{
	E_D.FromString(E_D_str);
	N.FromString(N_str);
}
void CCryptProtocol::Init(int mod)
{
	::LARGE_INTEGER pos;
	pos.QuadPart=0;
	bufferstream->Seek(pos,STREAM_SEEK_SET,nullptr);
	ReInitZstr(mod);
	aes.Init();
	m_mode=mod;
}
void CCryptProtocol::SetPackBack(UCHAR (&buf)[56],UINT len)
{
	ATLASSERT(m_mode==PackMode);
	ATLASSERT(len<=56);
	ZeroMemory(&headpack.packback,sizeof(headpack.packback));
	memcpy(headpack.packback,buf,len);
}
void CCryptProtocol::WriteContent(void* buf,UINT len)
{
	int res;
	ATLASSERT(m_mode==PackMode);
	zstr.avail_in=(uInt)len;
	zstr.next_in=(Bytef*)buf;
	while(true)
	{
		zstr.avail_out=sizeof(pubbuf);
		zstr.next_out=(Bytef*)pubbuf;
		zstr.total_out=0;

		res=deflate(&zstr,Z_SYNC_FLUSH);
		if(res==Z_BUF_ERROR)
		{
			if(zstr.avail_in==0) break;
		}
		ATLASSERT(res==Z_OK);
		bufferstream->Write(pubbuf,zstr.total_out,nullptr);
	}
}
void CCryptProtocol::WriteEnd()
{
	int res;
	ATLASSERT(m_mode==PackMode);
	zstr.avail_out=sizeof(pubbuf);
	zstr.next_out=(Bytef*)pubbuf;
	zstr.total_out=0;
	res=deflate(&zstr, Z_FINISH);
	if(Z_STREAM_END==res)
	{
		bufferstream->Write(pubbuf,zstr.total_out,nullptr);
		headpack.srclen=bufferstream->GetBufferSize();
	}
}

WSABUF* CCryptProtocol::BuildPack(int packmod,size_t &wsabufcount)
{
	ATLASSERT(m_mode==PackMode);
	m_pack_mod=packmod;
	wsabufs.clear();
	switch(m_pack_mod)
	{
	case RSAPACK:
		{
			headpack.cryptlen=UPROUND(headpack.srclen,16);
			bufferstream->Write(pubbuf,headpack.cryptlen-headpack.srclen,nullptr);
			CRandom32 rand32;
			for(int i=0;i<_countof(headpack.aeskey);i++)headpack.aeskey[i]=(USHORT)rand32.Next();
			int encodelen=bufferstream->GetBufferSize();
			aes.SetKey(headpack.aeskey,sizeof(headpack.aeskey));
			aes.EndcodeDate(bufferstream->GetBuffer(),encodelen);
			sha1.Reset();
			sha1.Input((const UCHAR*)bufferstream->GetBuffer(),bufferstream->GetBufferSize());
			sha1.Result(headpack.sha1);

			headpack.headcrc=crc32(0,0,0);
			headpack.headcrc=crc32(headpack.headcrc,(Bytef*)&headpack,sizeof(headpack)-8-sizeof(ULONG));

			CBigInt crypttemp;
			crypttemp.SetByte((const UCHAR*)&headpack,sizeof(headpack));
			crypted=crypttemp.RsaTrans(E_D,N);

			WSABUF tempbuf;
			tempbuf.buf=(CHAR*)&m_pack_mod;
			tempbuf.len=1;
			wsabufs.push_back(tempbuf);
			tempbuf.buf=(CHAR*)crypted.GetByte(encodelen);
			tempbuf.len=128;
			wsabufs.push_back(tempbuf);
			tempbuf.buf=(CHAR*)bufferstream->GetBuffer();
			tempbuf.len=bufferstream->GetBufferSize();
			wsabufs.push_back(tempbuf);
		}break;
	case NORMAL:
		{
			headpack.srclen=bufferstream->GetBufferSize();

			WSABUF tempbuf;
			tempbuf.buf=(CHAR*)&m_pack_mod;
			tempbuf.len=1;
			wsabufs.push_back(tempbuf);
			tempbuf.buf=(CHAR*)&headpack.srclen;
			tempbuf.len=4;
			wsabufs.push_back(tempbuf);
			tempbuf.buf=(CHAR*)bufferstream->GetBuffer();
			tempbuf.len=bufferstream->GetBufferSize();
			wsabufs.push_back(tempbuf);
		}break;
	}
	wsabufcount=wsabufs.size();
	return &wsabufs[0];
}
bool CCryptProtocol::SavePack(void *buf,UINT &len)
{
	UINT needlen=0;
	for(size_t i=0;i<wsabufs.size();i++)
	{
		needlen+=wsabufs[i].len;
	}
	if(len<needlen)
	{
		len=needlen;
		return false;
	}
	len=needlen;
	for(size_t i=0;i<wsabufs.size();i++)
	{
		memcpy(buf,wsabufs[i].buf,wsabufs[i].len);
		buf=((char*)buf)+wsabufs[i].len;
	}
	return true;
}
int CCryptProtocol::RecvMode(UCHAR mod)
{
	ATLASSERT(UnPackMode==m_mode);
	m_pack_mod=mod;
	switch(m_pack_mod)
	{
	case RSAPACK:
		{
			return 128;
		}break;
	case NORMAL:
		{
			return 4;
		}break;
	}
	return -1;
}
int CCryptProtocol::RecvHeadPack(void* buf,int buflen)
{
	ATLASSERT(UnPackMode==m_mode);
	switch(m_pack_mod)
	{
	case RSAPACK:
		{
			ATLASSERT(buflen=128);
			CBigInt crypttemp;
			crypttemp.SetByte((const UCHAR*)buf,buflen);
			crypted=crypttemp.RsaTrans(E_D,N);

			int templen;
			const UCHAR* decrypted=crypted.GetByte(templen);
			if(templen>sizeof(headpack)) return -1;
			memcpy(&headpack,decrypted,templen);

			ULONG crc_check=crc32(0,0,0);
			crc_check=crc32(crc_check,(Bytef*)&headpack,sizeof(headpack)-8-sizeof(ULONG));
			if(crc_check!=headpack.headcrc) return -1;
			if(headpack.srclen>headpack.cryptlen) return -1;
			return headpack.cryptlen;
		}break;
	case NORMAL:
		{
			ATLASSERT(buflen=4);
			return *(int*)buf;
		}break;
	}
	return -1;
}
bool CCryptProtocol::RecvMainBody(void* buf,int buflen)
{
	ATLASSERT(UnPackMode==m_mode);
	switch(m_pack_mod)
	{
	case RSAPACK:
		{
			sha1.Reset();
			if(!sha1.Input((const UCHAR*)buf,buflen)) return false;
			Sha1Value shavalue;
			sha1.Result(shavalue);
			if(memcmp(shavalue,headpack.sha1,sizeof(Sha1Value))!=0) return false;
			if(buflen!=headpack.cryptlen) return false;
			
			aes.SetKey(headpack.aeskey,sizeof(headpack.aeskey));
			aes.DecodeData(buf,buflen);
			
			::ULARGE_INTEGER sz;
			sz.QuadPart=0;
			bufferstream->SetSize(sz);
			int res;
			zstr.avail_in=(uInt)headpack.srclen;
			zstr.next_in=(Bytef*)buf;
			while(true)
			{
				zstr.avail_out=sizeof(pubbuf);
				zstr.next_out=(Bytef*)pubbuf;
				zstr.total_out=0;

				res=inflate(&zstr,Z_SYNC_FLUSH);
				if(res==Z_STREAM_END)
				{
					bufferstream->Write(pubbuf,zstr.total_out,nullptr);
					break;
				}
				else if(res==Z_OK)
				{
					bufferstream->Write(pubbuf,zstr.total_out,nullptr);
				}
				else
					return false;
			}
		}break;
	case NORMAL:
		{
			::ULARGE_INTEGER sz;
			sz.QuadPart=0;
			bufferstream->SetSize(sz);
			int res;
			zstr.avail_in=(uInt)buflen;
			zstr.next_in=(Bytef*)buf;
			while(true)
			{
				zstr.avail_out=sizeof(pubbuf);
				zstr.next_out=(Bytef*)pubbuf;
				zstr.total_out=0;

				res=inflate(&zstr,Z_SYNC_FLUSH);
				if(res==Z_STREAM_END)
				{
					bufferstream->Write(pubbuf,zstr.total_out,nullptr);
					break;
				}
				else if(res==Z_OK)
				{
					bufferstream->Write(pubbuf,zstr.total_out,nullptr);
				}
				else
					return false;
			}
		}break;
	}
	return true;
}
#include "GZipCaps.h"

enum{
	ASCII_FLAG=0x01, /* bit 0 set: file probably ascii text */
	HEAD_CRC=0x02, /* bit 1 set: header CRC present */
	EXTRA_FIELD=0x04, /* bit 2 set: extra field present */
	ORIG_NAME=0x08,/* bit 3 set: original file name present */
	COMMENT=0x10, /* bit 4 set: file comment present */
	RESERVED=0xE0 /* bits 5..7: reserved */
};
enum{MagicCheck,MethodRead,
FlagsRead,MTimeRead,XFlagRead,OSCodeRead,ExtrafieldRead,
ExtrafieldReadLenLow,ExtrafieldReadLenHigh,
ExtrafieldSkip,OrignameRead,
CommentRead,HeadCRCRead,HeadCRCRead2};

int CGZCap::Destroy()
{
	int err = Z_OK;
	if (m_zstream.state != nullptr) {
		err = deflateEnd(&(m_zstream));
	}
	return err;
}
void CGZCap::Write(LPCSTR buf,ULONG bufsz)
{
	ULONG writen=0;
	HRESULT res;
	while(bufsz)
	{
		res=out->Write(buf,bufsz,&writen);
		if(S_OK==res)
		{
			buf+=writen;
			bufsz-=writen;
		}
		else
			throw res;
	}
}
void CGZCap::WriteLong (uLong x)
{
	char buf[4];
	for(int n = 0; n < 4; n++) {
		buf[n]=(unsigned char)(x & 0xff);
		x >>= 8;
	}
	Write(buf,4);
}
CGZCap::CGZCap(int nLevel,int nStrategy)
{
	ZeroMemory(&m_zstream,sizeof(m_zstream));
	m_zstream.zalloc = (alloc_func)0;
	m_zstream.zfree = (free_func)0;
	m_zstream.opaque = (voidpf)0;
	if(Z_OK != deflateInit2(&(m_zstream), nLevel,Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, nStrategy))
		ATLASSERT(0);
}
bool CGZCap::Init(IStream *output)
{
	started=false;
	out=output;
	m_zstream.next_in = Z_NULL;
	m_zstream.next_out = Z_NULL;
	m_zstream.avail_in = 0;
	m_zstream.avail_out = 0;
	m_crc = crc32(0L, Z_NULL, 0);
	int err = deflateReset(&(m_zstream));
	if(err!=Z_OK)
	{
		Destroy();
		return false;
	}
	return true;
}
void CGZCap::Reset()
{
	int res=deflateReset(&m_zstream);
	if(res!=Z_OK)
		throw res;
	m_crc = crc32(0L, Z_NULL, 0);
	started=false;
}
CGZCap::~CGZCap()
{
	Destroy();
}
void CGZCap::Input(LPCSTR buf,int bufsz)
{
	if(m_zstream.state==nullptr) return;
	if(started==false)
	{
		started=true;
		const UCHAR header[10]={0x1f,0x8b,Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, OS_CODE};
		Write((LPCSTR)header,sizeof(header));
	}

	m_crc = crc32(m_crc, (const Bytef *)buf, bufsz);
	m_zstream.avail_in=bufsz;
	m_zstream.next_in=(Bytef*)buf;
	while(m_zstream.avail_in>0)
	{
		m_zstream.avail_out=sizeof(m_buffer);
		m_zstream.next_out=(Bytef*)m_buffer;
		int res = deflate(&m_zstream,Z_FULL_FLUSH);
		if(res!=Z_OK) throw res;
		Write(m_buffer,sizeof(m_buffer)-m_zstream.avail_out);
	}
}
void CGZCap::Finish()
{
	m_zstream.avail_in=0;
	m_zstream.next_in=nullptr;
	while(true)
	{
		m_zstream.avail_out=sizeof(m_buffer);
		m_zstream.next_out=(Bytef*)m_buffer;
		int res=deflate(&(m_zstream), Z_FINISH);
		if(res==Z_OK)
		{
			Write(m_buffer,sizeof(m_buffer)-m_zstream.avail_out);
		}
		else if(res==Z_STREAM_END)
		{
			Write(m_buffer,sizeof(m_buffer)-m_zstream.avail_out);
			break;
		}
		else
			throw res;
	}
	WriteLong(m_crc);
	WriteLong(m_zstream.total_in);
}


void CUGZCap::Destroy()
{
	if (m_zstream.state != nullptr) {
		int err = inflateEnd(&(m_zstream));
		if(Z_OK!=err) throw err;
	}
}
void CUGZCap::Write(LPCSTR buf,ULONG bufsz)
{
	ULONG writen=0;
	HRESULT res;
	while(bufsz)
	{
		res=out->Write(buf,bufsz,&writen);
		if(S_OK==res)
		{
			buf+=writen;
			bufsz-=writen;
		}
		else
			throw res;
	}
}
CUGZCap::CUGZCap()
{
	ZeroMemory(&m_zstream,sizeof(m_zstream));

	m_zstream.zalloc = (alloc_func)0;
	m_zstream.zfree = (free_func)0;
	m_zstream.opaque = (voidpf)0;
	if(Z_OK!=inflateInit2(&(m_zstream), -MAX_WBITS))
		ATLASSERT(false);
}
bool CUGZCap::Init(IStream* output)
{
	out=output;
	
	m_zstream.next_in = Z_NULL;
	m_zstream.next_out = Z_NULL;
	m_zstream.avail_in = m_zstream.avail_out = 0;
	m_crc = crc32(0L, Z_NULL, 0);
	int  err = inflateReset(&(m_zstream));
	if (err != Z_OK)
	{
		return false;
	}
	return true;
}
void CUGZCap::Reset()
{
	m_crc = crc32(0L, Z_NULL, 0);
	int res=inflateReset(&(m_zstream));
	if(res!=Z_OK) throw res;
}
CUGZCap::~CUGZCap()
{
	Destroy();
}
bool CUGZCap::Input(LPCSTR buff,int bufflen,LPCSTR *tailcheckstart)
{
	int res;
	m_zstream.avail_in=bufflen;
	m_zstream.next_in=(Bytef*)buff;
	while(m_zstream.avail_in>0)
	{
		m_zstream.next_out=(Bytef*)m_buffer;
		m_zstream.avail_out=sizeof(m_buffer);
		res=inflate(&(m_zstream), Z_FULL_FLUSH);
		if(res!=Z_OK)
		{
			if(res==Z_STREAM_END)
			{
				int returnlen=sizeof(m_buffer)-m_zstream.avail_out;
				m_crc = crc32(m_crc, (Bytef*)m_buffer, returnlen);
				Write(m_buffer,returnlen);
				if(tailcheckstart)
				{
					*tailcheckstart=(LPCSTR)(m_zstream.next_in);
				}
				return false;
			}
			else
				throw res;
		}
		else
		{
			int returnlen=sizeof(m_buffer)-m_zstream.avail_out;
			m_crc = crc32(m_crc, (Bytef*)m_buffer, returnlen);
			Write(m_buffer,returnlen);
		}
	}
	return true;
}
bool CUGZCap::TailCheck(LPCSTR tailcheckstart,int size)
{
	if(size<(sizeof(uLong)*2)) return false;
	const UCHAR* temp=(const UCHAR*)tailcheckstart;
	uLong record_crc=0;
	uLong record_sz=0;
	record_crc = (uLong)*temp++;
	record_crc|= ((uLong)*temp++)<<8;
	record_crc|= ((uLong)*temp++)<<16;
	record_crc|= ((uLong)*temp++)<<24;
	record_sz=(uLong)*temp++;
	record_sz|= ((uLong)*temp++)<<8;
	record_sz|= ((uLong)*temp++)<<16;
	record_sz|= ((uLong)*temp++)<<24;
	return record_crc==m_crc && record_sz==m_zstream.total_out;
}

bool CGZHeadSkiper::CheckNext(UCHAR c)
{
	switch(stat)
	{
	case MagicCheck:
		{
			static const UCHAR gz_magic[2] = {0x1f, 0x8b};
			if(c!=gz_magic[count])
			{
				SetError();
				return false;
			}
			count++;
			if(count==2)
			{
				stat=MethodRead;
				count=0;
			}
		}break;
	case MethodRead:
		{
			method=c;
			stat=FlagsRead;
		}break;
	case FlagsRead:
		{
			flags=c;
			if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
				SetError();
				return false;
			}
			stat=MTimeRead;
			count=0;
		}break;
	case MTimeRead:
		{
			mtime|=((UINT)c)<<(8*count);
			count++;
			if(count==4)
			{
				count=0;
				stat=XFlagRead;
			}
		}break;
	case XFlagRead:
		{
			xflag=c;
			stat=OSCodeRead;
		}break;
	case OSCodeRead:
		{
			oscode=c;
			if ((flags & EXTRA_FIELD) != 0)
				stat=ExtrafieldRead;
			else if ((flags & ORIG_NAME) != 0)
				stat=OrignameRead;
			else if ((flags & COMMENT) != 0)
				stat=CommentRead;
			else if ((flags & HEAD_CRC) != 0)
			{
				stat=HeadCRCRead;
			}
			else return false;
		}break;
	case ExtrafieldRead:
		{
			count=(UINT)c;
			stat=ExtrafieldReadLenHigh;
		}break;
	case ExtrafieldReadLenHigh:
		{
			count|=((UINT)c)<<8;
			stat=ExtrafieldSkip;
		}break;
	case ExtrafieldSkip:
		{
			count--;
			if(count==0)
			{
				if ((flags & ORIG_NAME) != 0)
					stat=OrignameRead;
				else if ((flags & COMMENT) != 0)
					stat=CommentRead;
				else if ((flags & HEAD_CRC) != 0)
				{
					stat=HeadCRCRead;
				}
				else return false;
			}
		}break;
	case OrignameRead:
		{
			if(c!=0)
			{
				origname.AppendChar(c);
			}
			else
			{
				if ((flags & COMMENT) != 0)
					stat=CommentRead;
				else if ((flags & HEAD_CRC) != 0)
				{
					stat=HeadCRCRead;
				}
				else return false;
			}
		}break;
	case CommentRead:
		{
			if(c!=0)
			{
				comment.AppendChar(c);
			}
			else
			{
				if ((flags & HEAD_CRC) != 0)
				{
					stat=HeadCRCRead;
				}
				else return false;
			}
		}
	case HeadCRCRead:
		{
			headcrc[0]=c;
			stat=HeadCRCRead2;
		}break;
	case HeadCRCRead2:
		{
			headcrc[1]=c;
			return false;
		}break;
	}
	return true;
}
void CGZHeadSkiper::Init()
{
	stat=MagicCheck;
	count=0;
	error=false;
	mtime=0;
	ZeroMemory(headcrc,sizeof(headcrc));
}
CGZHeadSkiper::CGZHeadSkiper()
{
	Init();
}
bool CGZHeadSkiper::Input(LPCSTR buffer,int len,LPCSTR* headend)
{
	while(len)
	{
		if(CheckNext((UCHAR)*buffer)==false)
		{
			if(!IsError())
			{
				if(headend)
				{
					*headend=buffer+1;
				}
			}
			return false;
		}
		len--;
		buffer++;
	}
	return true;

}
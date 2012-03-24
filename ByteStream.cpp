#include "ByteStream.h"
#include "cexarray.h"
static const int uproundbase=512; 
ByteStream::ByteStream():bytes(nullptr),nowsize(0),nowpos(0)
{
}
ByteStream::~ByteStream()
{
	if(bytes)
		free(bytes);
}

HRESULT STDMETHODCALLTYPE ByteStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	ULONG toread=(ULONG)min(nowsize-nowpos,cb);
	if(toread==0)
	{ 
		if(pcbRead)*pcbRead=0;
		return S_OK;
	}
	memcpy(pv,bytes+nowpos,toread);
	nowpos+=toread;
	if(pcbRead!=nullptr)
		*pcbRead=toread;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ByteStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
	if(nowsize-nowpos<cb)
	{
		int newsize=nowpos+cb;
		char *newbytes=(char*)realloc(bytes,UPROUND(newsize,uproundbase));
		if(newbytes!=nullptr)
		{
			bytes=newbytes;
			nowsize=newsize;
		}
		else
		{
			return E_FAIL;
		}
	}
	memcpy(bytes+nowpos,pv,cb);
	nowpos+=cb;
	if(pcbWritten!=nullptr)
		*pcbWritten=cb;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ByteStream::SetSize(ULARGE_INTEGER size)
{ 
	if(size.QuadPart==0)
	{
		nowsize=0;
		nowpos=0;
		if(bytes)
		{
			free(bytes);
			bytes=nullptr;
		}
		return S_OK;
	}
	ULONG newsize=(ULONG)size.QuadPart;
	char* newbytes=(char*)realloc(bytes,UPROUND(newsize,uproundbase));
	if(newbytes)
	{
		bytes=newbytes;
		nowsize=newsize;
		nowpos=min(nowsize-1,nowpos);
		return S_OK;
	}
	else
		return E_FAIL;
}

HRESULT STDMETHODCALLTYPE ByteStream::CopyTo(IStream* pstm,
		ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten) 
{ 
	ULONG totrans=min(nowsize-nowpos,(ULONG)cb.QuadPart);
	ULONG writen;
	if(S_OK==pstm->Write(bytes+nowpos,totrans,&writen))
	{
		nowpos+=writen;
	}
	if(pcbRead)
	{
		(*pcbRead).QuadPart=writen;
	}
	if(pcbWritten)
	{
		(*pcbWritten).QuadPart=writen;
	}
	return S_OK;   
}

HRESULT STDMETHODCALLTYPE ByteStream::Commit(DWORD)                                      
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE ByteStream::Revert(void)                                       
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE ByteStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)              
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE ByteStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)            
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE ByteStream::Clone(IStream ** ppvstream)                                  
{ 
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ByteStream::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
											ULARGE_INTEGER* lpNewFilePointer)
{
	switch(dwOrigin)
	{
	case STREAM_SEEK_SET:
		{
			nowpos=(ULONG)liDistanceToMove.QuadPart;
		}
		break;
	case STREAM_SEEK_CUR:
		{
			nowpos+=(ULONG)liDistanceToMove.QuadPart;
		}
		break;
	case STREAM_SEEK_END:
		{
			nowpos=nowsize-(ULONG)liDistanceToMove.QuadPart;
		}
		break;
	default:   
		return STG_E_INVALIDFUNCTION;
		break;
	}
	nowpos=max(0,min(nowsize,nowpos));
	if(lpNewFilePointer)
	{
		lpNewFilePointer->QuadPart=nowpos;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ByteStream::Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
{
	if(grfStatFlag!=STATFLAG_NONAME)
	{
		pStatstg->pwcsName=(LPOLESTR)CoTaskMemAlloc( 2);
		*pStatstg->pwcsName=0;
	}
	pStatstg->cbSize.QuadPart=nowsize;
	return S_OK;
}

LPVOID STDMETHODCALLTYPE ByteStream::GetBuffer()
{
	return bytes;
}
ULONG STDMETHODCALLTYPE ByteStream::GetBufferSize()
{
	return nowsize;
}

HRESULT ByteStream::CreateInstanse(const IID &id,void** vp)
{
	ByteStream* newone=new CComObjectNoLock<ByteStream>();
	if(newone->QueryInterface(id,vp)!=S_OK)
	{
		delete newone;
		return E_FAIL;
	}
	return S_OK;
}
#include "DeflateStream.h"

bool CDeflateStream::Init(bool deflatemode,IStream* base)
{
	dmode=deflatemode;
	basestream=base;
	bufused=0;
	useable=false;

	ZeroMemory(&stream,sizeof(stream));
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	int res;
	if(dmode)
	{
		res=deflateInit(&stream,Z_BEST_COMPRESSION);
		if(res!=Z_OK)
			return false;
	}
	else
	{
		res=inflateInit(&stream);
		if(res!=Z_OK)
			return false;
	}
	return true;
}
CDeflateStream::~CDeflateStream()
{
	Commit(STGC_DEFAULT);
	int z_res;
	if(dmode)
	{
		z_res=deflateEnd(&stream);
	}
	else
	{
		z_res=inflateEnd(&stream);
	}
	ATLASSERT(z_res==Z_OK);
}
LRESULT CDeflateStream::CreateDeflateStream(bool deflatemode,IStream* basestream,IStream **outstream)
{
	CComObjectNoLock<CDeflateStream> *ds=new CComObjectNoLock<CDeflateStream>();
	if(ds==nullptr)
		return E_FAIL;
	if(ds->Init(deflatemode,basestream))
	{
		if(S_OK!=ds->QueryInterface(__uuidof(IStream),(void**)outstream))
		{
			delete ds;
			return E_FAIL;
		}
		else
		{
			ds->EnUseable();
			return S_OK;
		}
	}
	else
	{
		if(ds)
			delete ds;
		return E_FAIL;
	}
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	if(dmode) return E_NOTIMPL;
	else
	{
		if(!useable)
		{
			if(pcbRead)
				*pcbRead=0;
			return S_OK;
		}
		stream.avail_out=cb;
		stream.next_out=(Bytef*)pv;
		stream.total_out=0;
		LRESULT res=S_OK;
		while(true)
		{
			if(stream.avail_in==0)
			{
				res=basestream->Read(buffer,sizeof(buffer),&bufused);
				if(res==S_OK)
				{
					stream.avail_in=bufused;
					stream.next_in=(Bytef*)buffer;
				}
				else
				{
					return res;
				}
			}
			int z_res=inflate(&stream,Z_SYNC_FLUSH);
			if(z_res==Z_OK || z_res==Z_STREAM_END)
			{
				if(z_res==Z_STREAM_END)
				{
					if(pcbRead)
						*pcbRead=stream.total_out;
					res=S_OK;
					useable=false;
					break;
				}
			}
			else if(z_res==Z_BUF_ERROR)
			{
				if(stream.avail_out==0)
				{
					if(pcbRead)
						*pcbRead=stream.total_out;
					res=S_OK;
					break;
				}
				else if(stream.avail_in==0)
				{
					continue;
				}
				else
				{
					res=E_FAIL;
					break;
				}
			}
		}
		return res;
	}
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
	if(dmode)
	{
		const char *kk=(const char*)pv;
		if(!useable) return E_FAIL;
		LRESULT res=S_OK;
		int z_res=Z_OK;
		stream.next_in=(Bytef*)pv;
		stream.avail_in=cb;
		stream.total_in=0;

		while(true)
		{
			stream.next_out=(Bytef*)buffer;
			stream.avail_out=sizeof(buffer);
			stream.total_out=0;
			z_res=deflate(&stream, Z_SYNC_FLUSH);

			if(z_res==Z_OK)
			{
				ULONG w;
				if(S_OK!=basestream->Write(buffer,stream.total_out,&w))
				{
					useable=false;
					return E_FAIL;
				}
			}
			else if(z_res==Z_BUF_ERROR)
			{
				if(stream.avail_in==0)
				{
					ULONG w;
					if(S_OK==basestream->Write(buffer,stream.total_out,&w))
					{
						if(pcbWritten)
							(*pcbWritten)=stream.total_in;
						return S_OK;
					}
					else
					{
						useable=false;
						return E_FAIL;
					}
				}
				else if(stream.avail_out==0)
					continue;
				else
				{
					useable=false;
					return E_FAIL;
				}
			}
			else
			{
				useable=false;
				return E_FAIL;
			}
		}
	}
	else
		return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CDeflateStream::SetSize(ULARGE_INTEGER size)
{ 
	return E_NOTIMPL;  
}
HRESULT STDMETHODCALLTYPE CDeflateStream::CopyTo(IStream* pstm,
												 ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten) 
{ 
	return E_NOTIMPL; 
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Commit(DWORD opt)                                      
{	
	if(dmode)
	{
		if(opt!=STGC_DEFAULT || !useable) return E_FAIL;
		useable=false;
		stream.next_out=(Bytef*)buffer;
		stream.avail_out=sizeof(buffer);
		stream.total_out=0;

		int z_res=deflate(&stream, Z_FINISH);
		if(z_res==Z_STREAM_END)
		{
			if(S_OK==basestream->Write(buffer,stream.total_out,0))
			{
				return S_OK;
			}
			else
			{
				return E_FAIL;
			}
		}
		else
		{
			return E_FAIL;
		}
	}
	else
		return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Revert(void)                                       
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE CDeflateStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)              
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE CDeflateStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)            
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Clone(IStream ** ppvstream)                                  
{ 
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
											   ULARGE_INTEGER* lpNewFilePointer)
{
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CDeflateStream::Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
{
	return E_NOTIMPL;
}

#include "UnZipStreams.h"


bool CUnZipStream::Init(IStream* base)
{
	unzipcap.Reset(base);
	return true;
}
LRESULT CUnZipStream::CreateInstense(IStream* unzip_to,IStream **input_ziped)
{
	CComObjectNoLock<CUnZipStream> *ds=new CComObjectNoLock<CUnZipStream>();
	if(ds==nullptr)
		return E_FAIL;
	if(ds->Init(unzip_to))
	{
		if(S_OK==ds->QueryInterface(__uuidof(IStream),(void**)input_ziped))
		{
			return S_OK;
		}
	}
	delete ds;
	return E_FAIL;
}
HRESULT STDMETHODCALLTYPE CUnZipStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
	int res=unzipcap.Write(pv,cb,(size_t*)pcbWritten);
	if(res==-1) return E_FAIL;
	if(pcbWritten)
		*pcbWritten=cb;
	return S_OK;
}

LRESULT CInflateStream::CreateInstense(IStream* unzip_to,IStream **input_ziped)
{
	CComObjectNoLock<CInflateStream> *ds=new CComObjectNoLock<CInflateStream>();
	if(ds==nullptr)
		return E_FAIL;
	if(ds->Init(unzip_to))
	{
		if(S_OK==ds->QueryInterface(__uuidof(IStream),(void**)input_ziped))
		{
			return S_OK;
		}
	}
	delete ds;
	return E_FAIL;
}
bool CInflateStream::Init(IStream* base)
{
	unzipcap.Reset(base);
	return true;
}
HRESULT STDMETHODCALLTYPE CInflateStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
	int res=unzipcap.Write(pv,cb,(size_t*)pcbWritten);
	if(res==-1) return E_FAIL;
	if(pcbWritten)
		*pcbWritten=cb;
	return S_OK;
}

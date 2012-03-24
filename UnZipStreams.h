#pragma once

#include "gzipcap2.h"
#include <atlcom.h>

class CUnZipStream:public CComObjectRootEx<CComSingleThreadModel>,public IStream
{
private:
	enum{ProcHead,ProcBody,ProcTail};
	GUNZIPCap2 unzipcap;
protected:
	bool Init(IStream* base);
public:
	static LRESULT CreateInstense(IStream* unzip_to,IStream **input_ziped);
protected:
	CHAR tailbuf[8];
	int tailin;
	HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
protected:
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE CopyTo(IStream* pstm,
		ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Commit(DWORD opt){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Revert(void){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Clone(IStream ** ppvstream){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
		ULARGE_INTEGER* lpNewFilePointer){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag){return E_NOTIMPL;}

	BEGIN_COM_MAP(CUnZipStream)
		COM_INTERFACE_ENTRY(ISequentialStream)
		COM_INTERFACE_ENTRY(IStream)
	END_COM_MAP()
};
class CInflateStream:public CComObjectRootEx<CComSingleThreadModel>,public IStream
{
protected:
	BEGIN_COM_MAP(CInflateStream)
		COM_INTERFACE_ENTRY(ISequentialStream)
		COM_INTERFACE_ENTRY(IStream)
	END_COM_MAP()
public:
	static LRESULT CreateInstense(IStream* unzip_to,IStream **input_ziped);
protected:
	InflateCap unzipcap;
	bool Init(IStream* base);
	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
	HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE CopyTo(IStream* pstm,
		ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Commit(DWORD opt){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Revert(void){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Clone(IStream ** ppvstream){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
		ULARGE_INTEGER* lpNewFilePointer){return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag){return E_NOTIMPL;}
};
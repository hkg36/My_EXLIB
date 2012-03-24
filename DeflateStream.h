#pragma once
#include <atlbase.h>
#include <atlcom.h>
#include <zlib.h>
class CDeflateStream:public CComObjectRootEx<CComSingleThreadModel>,public IStream
{
private:
	z_stream stream;
	bool dmode;
	bool useable;
	Bytef buffer[128];
	ULONG bufused;
	CComPtr<IStream> basestream;
protected:
	bool Init(bool deflatemode,IStream* base);
	~CDeflateStream();
	void EnUseable(){useable=true;}
public:
	static LRESULT CreateDeflateStream(bool deflatemode,IStream* basestream_ziped,IStream **outstream_unziped);
protected:
	 virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
protected:
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size);
    
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream* pstm,
		ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten);
    
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD opt);
    virtual HRESULT STDMETHODCALLTYPE Revert(void);
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) ;
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppvstream);
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
        ULARGE_INTEGER* lpNewFilePointer);
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);

	BEGIN_COM_MAP(CDeflateStream)
		COM_INTERFACE_ENTRY(ISequentialStream)
		COM_INTERFACE_ENTRY(IStream)
	END_COM_MAP()
};

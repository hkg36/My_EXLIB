#pragma once
#include <atlbase.h>
#include <atlcom.h>

MIDL_INTERFACE("E22A0285-C480-4b3a-800D-8A0160060699")
IMemoryStream:public IStream
{
	virtual LPVOID STDMETHODCALLTYPE GetBuffer()=0;
	virtual ULONG STDMETHODCALLTYPE GetBufferSize()=0;
};
class ByteStream:public CComObjectRootEx<CComSingleThreadModel>,
	public IMemoryStream
{
protected:
	BEGIN_COM_MAP(ByteStream)
		COM_INTERFACE_ENTRY(ISequentialStream)
		COM_INTERFACE_ENTRY(IStream)
		COM_INTERFACE_ENTRY(IMemoryStream)
	END_COM_MAP()
protected:
	char *bytes;
	ULONG nowsize;
	ULONG nowpos;
protected:
	ByteStream();
	~ByteStream();
protected:
	virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
protected:
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size);
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream* pstm,
		ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten);
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD);
    virtual HRESULT STDMETHODCALLTYPE Revert(void);
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppvstream);
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
        ULARGE_INTEGER* lpNewFilePointer);
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);
	virtual LPVOID STDMETHODCALLTYPE GetBuffer();
	virtual ULONG STDMETHODCALLTYPE GetBufferSize();
public:
	static HRESULT CreateInstanse(const IID &id,void** vp);
	template<class TYPE>
	static HRESULT CreateInstanse(TYPE** vp)
	{
		return CreateInstanse(__uuidof(TYPE),(void**)vp);
	}
};
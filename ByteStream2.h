#pragma once
#include <WinSock2.h>
#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <vector>

MIDL_INTERFACE("7EF85263-564C-47f8-A05F-6CC954C827E9")
IMemoryStream2:public IStream
{
	virtual HRESULT STDMETHODCALLTYPE GetWSABUFList(WSABUF *list,size_t *len)=0;
};
class ByteStream2:
	public CComObjectRootEx<CComSingleThreadModel>,
	public IMemoryStream2
{
protected:
	std::vector<CHAR*> buflist;
	size_t line_index;
	size_t sub_index;
	size_t end_line_index;
	size_t end_sub_index;
	DWORD chunksize;
	ByteStream2();
	~ByteStream2();
	CHAR* AllocChunck();
	void FreeChunk(CHAR* p);
protected:
	BEGIN_COM_MAP(ByteStream2)
		COM_INTERFACE_ENTRY(ISequentialStream)
		COM_INTERFACE_ENTRY(IStream)
		COM_INTERFACE_ENTRY(IMemoryStream2)
	END_COM_MAP()
protected:
	void CheckToRead(DWORD &canread);
	void PositionCheck();
	void CheckReadUpdatePos();
	HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
	void CheckWriteUpdateEnd();
	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
protected:
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size);
	HRESULT STDMETHODCALLTYPE CopyTo(IStream* pstm,
		ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten);
	HRESULT STDMETHODCALLTYPE Commit(DWORD);
	HRESULT STDMETHODCALLTYPE Revert(void);
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
	HRESULT STDMETHODCALLTYPE Clone(IStream ** ppvstream);
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
		ULARGE_INTEGER* lpNewFilePointer);
	HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);
protected:
	HRESULT STDMETHODCALLTYPE GetWSABUFList(WSABUF *list,size_t *len);
public:
	static HRESULT CreateInstanse(const IID &id,void** vp);
	template<class TYPE>
	static HRESULT CreateInstanse(TYPE** vp)
	{
		return CreateInstanse(__uuidof(TYPE),(void**)vp);
	}
};
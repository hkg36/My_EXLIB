#include "ByteStream2.h"


ByteStream2::ByteStream2():line_index(0),sub_index(0),end_line_index(0),end_sub_index(0)
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	chunksize=info.dwAllocationGranularity;
	buflist.push_back(AllocChunck());
}
ByteStream2::~ByteStream2()
{
	for(size_t i=0;i<buflist.size();i++)
	{
		FreeChunk(buflist[i]);
	}
}
CHAR* ByteStream2::AllocChunck()
{
	//#ifdef _DEBUG
	//		return (CHAR*)malloc(chunksize);
	//#else
	CHAR* temp=(CHAR*)VirtualAlloc(nullptr,chunksize,MEM_COMMIT | MEM_RESERVE|MEM_TOP_DOWN,PAGE_READWRITE);
	ATLASSERT(temp);
	VirtualLock(temp,chunksize);
	return temp;
	//#endif
}
void ByteStream2::FreeChunk(CHAR* p)
{
	//#ifdef _DEBUG
	//		free(p);
	//#else
	VirtualFree(p,0,MEM_RELEASE);
	//#endif
}

void ByteStream2::CheckToRead(DWORD &canread)
{
	if(line_index<end_line_index)
	{
		canread=chunksize-sub_index;
	}
	else if(line_index==end_line_index)
	{
		canread=end_sub_index-sub_index;
	}
	else
	{
		ATLASSERT(false);
	}
}
void ByteStream2::PositionCheck()
{
	if(line_index==end_line_index)
	{
		if(sub_index>end_sub_index)
		{
			sub_index=end_sub_index;
		}
	}
	else if(line_index>end_line_index)
	{
		line_index=end_line_index;
		sub_index=end_sub_index;
	}
}
void ByteStream2::CheckReadUpdatePos()
{
	if(sub_index==chunksize)
	{
		line_index++;
		sub_index=0;
	}
	PositionCheck();
}
HRESULT STDMETHODCALLTYPE ByteStream2::Read(void* pv, ULONG cb, ULONG* pcbRead)
{ 
	ULONG read=0;
	while(cb)
	{
		DWORD canread;
		CheckToRead(canread);
		if(canread==0) break;
		DWORD toread=min(canread,cb);
		memcpy(pv,buflist[line_index]+sub_index,toread);
		sub_index+=toread;
		read+=toread;
		cb-=toread;
		pv=((char*)pv)+toread;
		CheckReadUpdatePos();
	}
	if(pcbRead)
		*pcbRead=read;
	return S_OK;   
}
void ByteStream2::CheckWriteUpdateEnd()
{
	if(line_index==end_line_index)
	{
		if(sub_index>end_sub_index)
		{
			end_sub_index=sub_index;
		}
	}
	else if(line_index>end_line_index)
	{
		end_line_index=line_index;
		end_sub_index=sub_index;
	}
}
HRESULT STDMETHODCALLTYPE ByteStream2::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{ 
	ULONG writen=0;
	while(cb)
	{
		if(sub_index==chunksize)
		{
			if(line_index==buflist.size()-1)
			{
				CHAR *temp=AllocChunck();
				if(temp)
				{
					buflist.push_back(temp);
					sub_index=0;
					line_index++;
					CheckWriteUpdateEnd();
				}
				else
				{
					break;
				}
			}
			else
			{
				line_index++;
				sub_index=0;
				CheckWriteUpdateEnd();
			}
		}
		ULONG towrite=min(chunksize-sub_index,cb);
		memcpy(buflist[line_index]+sub_index,pv,towrite);
		pv=((char*)pv)+towrite;
		cb-=towrite;
		writen+=towrite;
		sub_index+=towrite;
		CheckWriteUpdateEnd();
	}
	if(pcbWritten)
		*pcbWritten=writen;
	return S_OK;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::SetSize(ULARGE_INTEGER size)
{ 
	size_t new_line_index=(size_t)size.QuadPart/chunksize;
	size_t new_sub_index=(size_t)size.QuadPart%chunksize;
	if(new_line_index>=buflist.size())
	{
		size_t toadd=new_line_index-buflist.size()+1;
		for(size_t i=0;i<toadd;i++)
		{
			buflist.push_back(AllocChunck());
		}
	}
	else if(new_line_index<buflist.size()-1)
	{
		size_t todel=buflist.size()-new_line_index-1;
		for(size_t i=0;i<todel;i++)
		{
			FreeChunk(buflist.back());
			buflist.pop_back();
		}
	}
	end_line_index=new_line_index;
	end_sub_index=new_sub_index;
	PositionCheck();
	return S_OK;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::CopyTo(IStream* pstm,
											  ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten)
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::Commit(DWORD)
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::Revert(void)
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::Clone(IStream ** ppvstream)
{ 
	return E_NOTIMPL;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
											ULARGE_INTEGER* lpNewFilePointer)
{ 
	ULONGLONG newpostoset=0;
	ULONGLONG nowpos=(ULONGLONG)chunksize*line_index+sub_index;
	ULONGLONG nowsize=(ULONGLONG)chunksize*end_line_index+end_sub_index;
	switch(dwOrigin)
	{
	case STREAM_SEEK_SET:
		{
			ULARGE_INTEGER pos;
			memcpy(&pos,&liDistanceToMove,sizeof(ULARGE_INTEGER));
			newpostoset=pos.QuadPart;
		}
		break;
	case STREAM_SEEK_CUR:
		{
			newpostoset=nowpos+liDistanceToMove.QuadPart;
		}
		break;
	case STREAM_SEEK_END:
		{
			newpostoset=nowsize-liDistanceToMove.QuadPart;
		}
		break;
	default:   
		return STG_E_INVALIDFUNCTION;
		break;
	}
	line_index=(size_t)newpostoset/chunksize;
	sub_index=(size_t)newpostoset%chunksize;
	CheckReadUpdatePos();
	if(lpNewFilePointer)
		lpNewFilePointer->QuadPart=(ULONGLONG)chunksize*line_index+sub_index;
	return S_OK;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::Stat(STATSTG* pStatstg, DWORD grfStatFlag)
{ 
	pStatstg->cbSize.QuadPart=(ULONGLONG)chunksize*end_line_index+end_sub_index;
	return S_OK;   
}
HRESULT STDMETHODCALLTYPE ByteStream2::GetWSABUFList(WSABUF *list,size_t *len)
{
	if(len==nullptr)
		return E_POINTER;
	if(list==nullptr)
	{
		*len=buflist.size();
	}
	else
	{
		size_t buflen=*len;
		if(buflen<buflist.size())
		{
			return E_FAIL;
		}
		for(size_t i=0;i<buflist.size()-1;i++)
		{
			list[i].buf=buflist[i];
			list[i].len=chunksize;
		}
		list[buflist.size()-1].buf=buflist[buflist.size()-1];
		list[buflist.size()-1].len=end_sub_index;
	}
	return S_OK;
}
HRESULT ByteStream2::CreateInstanse(const IID &id,void** vp)
{
	ByteStream2* newone=new CComObjectNoLock<ByteStream2>();
	if(newone->QueryInterface(id,vp)!=S_OK)
	{
		delete newone;
		return E_FAIL;
	}
	return S_OK;
}


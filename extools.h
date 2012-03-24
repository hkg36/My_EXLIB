#pragma once
#include "cexarray.h"
#include <atlcoll.h>
#include <atlcom.h>

class CRunOnlyOne
{
private:
	HANDLE handle;
	bool first;
public:
	CRunOnlyOne(LPCTSTR mutexname,bool auto_exit_when_not_first=false);
	bool IsFirst();
	~CRunOnlyOne();
};
class CGlobalAtom
{
private:
	ATOM atom;
public:
	CGlobalAtom():atom(0){}
	void Delete()
	{
		GlobalDeleteAtom(atom);
	}
	ATOM Create(LPCTSTR mutexname)
	{
		atom=GlobalFindAtom(mutexname);
		if(atom==0)
		{
			atom=GlobalAddAtom(mutexname);
		}
		return atom;
	}
	ATOM GetAtom(){return atom;}
};

HRESULT CreateComObject(LPCWSTR file,
	const CLSID &clsid,IUnknown *pUnknownOuter,DWORD dwClsContext,const IID &iid,void **ppv,bool CoCreateFirst=false);

class CSysErrorInfo
{
private:
	LPWSTR info;
public:
	CSysErrorInfo(DWORD id=GetLastError()):info(nullptr)
	{
		TransID(id);
	}
	~CSysErrorInfo()
	{
		if(info)
			LocalFree((HLOCAL)info);
	}
	LPWSTR TransID(DWORD errorid)
	{
		if(info)
		{
			LocalFree((HLOCAL)info);
			info=nullptr;
		}
		if(!FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
			nullptr,
			errorid,
			0,
			(LPWSTR)&info,
			0,
			nullptr))
		{
			if(info)
			{
				LocalFree((HLOCAL)info);
				info=nullptr;
			}
		}
		return info;
	}
	operator LPCWSTR()
	{
		return info;
	}
};

template<typename TaskType,class SubClass>
class CTaskProcessTmpl
{
private:
	typedef typename CTaskProcessTmpl<TaskType,SubClass> SelfClass;
	vector<HANDLE> threadlist;
	CAtlList<TaskType> tasklist[5];
	CCriticalLock lock;
	CExSemaphore waiter;
	bool keepwork;

	static unsigned __stdcall WorkThreadWarp ( void * param)
	{
		SelfClass *p= static_cast<SelfClass*>(param);
		return p->WorkThread();
	}
	unsigned  WorkThread ()
	{
		WorkThreadBegin();
		while(keepwork)
		{
			bool worktodo=false;
			TaskType onetask;
			waiter.Wait();
			if(keepwork)
			{
				CAutoCLock al(lock);
				for(int i=0;i<_countof(tasklist);i++)
				{
					if(!tasklist[i].IsEmpty())
					{
						onetask=tasklist[i].RemoveHead();
						worktodo=true;
						break;
					}
				}
			}else break;
			if(worktodo)
			{
				(static_cast<SubClass*>(this))->DoWork(onetask);
			}
		}
		WorkThreadEnd();
		return 0;
	}
	size_t max_task_count;
public:
	virtual void WorkThreadBegin(){};
	virtual void WorkThreadEnd(){};
	typedef typename TaskType TaskType;
	void SetMaxTaskQueue(size_t count){max_task_count=count;}
	size_t GetTackCount(){return tasklist.GetCount();}
	bool AddTask(TaskType &work,int priority=3)
	{
		ATLASSERT(priority<_countof(tasklist));
		if(tasklist[priority].GetCount()>=max_task_count) return false;
		lock.Lock();
		tasklist[priority].AddTail(work);
		lock.Unlock();
		waiter.Release();
		return true;
	}
	bool AddTaskIfEmpty(TaskType &work,int priority=3)
	{
		if(tasklist[priority].IsEmpty())
			return AddTask(work,priority);
		return false;
	}
	enum{GoNext,DropIt,ReplaceIt};
	typedef int (*CheckFunction)(TaskType &inqueue,TaskType &newone);
	bool AddTaskWithCheck(TaskType &work,CheckFunction fun,int priority=3)
	{
		ATLASSERT(priority<_countof(tasklist));
		ATLASSERT(fun);
		if(tasklist[priority].GetCount()>=max_task_count) return false;
		{
			CAutoCLock al(lock);
			POSITION pos=tasklist[priority].GetHeadPosition();
			while(pos)
			{
				TaskType& inqueueone=tasklist[priority].GetNext(pos);
				switch(fun(inqueueone,work))
				{
				case GoNext:break;
				case DropIt:return false;
				case ReplaceIt:
					{
						inqueueone=work;
						return true;
					}break;
				}
			}
			tasklist[priority].AddTail(work);
			waiter.Release();
			return true;
		}
	}
	CTaskProcessTmpl():keepwork(false),max_task_count((size_t)-1)
	{
	}
	~CTaskProcessTmpl()
	{
		Stop();
	}
	void Start(DWORD threadcount=0)
	{
		if(keepwork==true) return;
		keepwork=true;
		if(threadcount==0)
		{
			SYSTEM_INFO info;
			::GetSystemInfo(&info);
			threadcount=info.dwNumberOfProcessors*2;
		}
		for(DWORD i=0;i<threadcount;i++)
		{
			threadlist.push_back((HANDLE)_beginthreadex(0,0,WorkThreadWarp,this,0,0));
		}
	}
	void Stop()
	{
		if(keepwork==false) return;
		keepwork=false;
		waiter.Release(threadlist.size());
		::WaitForMultipleObjects(threadlist.size(),&threadlist[0],TRUE,INFINITE);
		for(size_t i=0;i<threadlist.size();i++)
		{
			CloseHandle(threadlist[i]);
		}
	}
};

class IWorkBase:public IPtrBase<IWorkBase>
{
public:
	virtual void ProcessWork()=0;
	virtual void NotifyProcessStart(){};
	virtual void NotifyProcessDone(DWORD process_time){process_time;};
	virtual ~IWorkBase(){}
};
typedef CIPtr<IWorkBase> LPWORKBASE;
class CWorkBaseProcessor:public CTaskProcessTmpl<LPWORKBASE,CWorkBaseProcessor>
{
public:
	bool AddTask(LPWORKBASE one,int priority=3)
	{
		if(one==nullptr) return false;
		return CTaskProcessTmpl<LPWORKBASE,CWorkBaseProcessor>::AddTask(one,priority);
	}
	void DoWork(TaskType &work)
	{
		work->NotifyProcessStart();
		DWORD starttime=::GetTickCount();
		work->ProcessWork();
		work->NotifyProcessDone(::GetTickCount()-starttime);
	}
};

class CMYW2A
{
protected:
	CMYW2A():m_psz( m_szBuffer ){}
public:
	CMYW2A(LPCWSTR psz):
	  m_psz( m_szBuffer )
	  {
		  Init( psz, CP_ACP,-1);
		  nLengthA-=1;
	  }
	  CMYW2A(LPCWSTR psz, UINT nCodePage):
	  m_psz( m_szBuffer )
	  {
		  Init( psz, nCodePage,-1);
		  nLengthA-=1;
	  }
	  ~CMYW2A()
	  {		
		  if(m_psz!=m_szBuffer)
			  free(m_psz);
	  }

	  operator LPSTR()
	  {
		  return( m_psz );
	  }
	  int Len()const{return nLengthA;}

protected:
	void Init( _In_opt_ LPCWSTR psz, _In_ UINT nConvertCodePage ,int pszlen)
	{
		if(pszlen==0)
		{
			*m_psz=0;
			nLengthA=0;
			return;
		}
		nLengthA=_countof(m_szBuffer);
		int res=::WideCharToMultiByte( nConvertCodePage, 0, psz, pszlen, m_psz, nLengthA, nullptr, nullptr );
		if (res==0)
		{
			if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
			{
				nLengthA = ::WideCharToMultiByte( nConvertCodePage, 0, psz, pszlen, nullptr, 0, nullptr, nullptr );
				m_psz=(LPSTR)malloc(nLengthA);
				res=::WideCharToMultiByte( nConvertCodePage, 0, psz, pszlen, m_psz, nLengthA, nullptr, nullptr );
				if(res!=0)
					nLengthA=res;
			}			
		}
		else
			nLengthA=res;
	}

public:
	LPSTR m_psz;
	char m_szBuffer[32];
	int nLengthA;
private:
	CMYW2A( const CMYW2A& );
	CMYW2A& operator=( const CMYW2A& );
};
class CMYW2AEX:public CMYW2A
{
public:
	CMYW2AEX(LPCWSTR psz,int len)
	{
		Init( psz, CP_ACP,len);
	}
	CMYW2AEX(LPCWSTR psz,int len, UINT nCodePage)
	{
		Init( psz, nCodePage,len);
	}
private:
	CMYW2AEX( const CMYW2AEX& );
	CMYW2AEX& operator=( const CMYW2AEX& );
};
class CMYA2W
{
protected:
	CMYA2W():m_psz( m_szBuffer ){}
public:
	CMYA2W(LPCSTR psz ):
	  m_psz( m_szBuffer )
	  {
		  Init( psz, CP_ACP ,-1);
		  nLengthW-=1;
	  }
	  CMYA2W(LPCSTR psz, UINT nCodePage ):
	  m_psz( m_szBuffer )
	  {
		  Init( psz, nCodePage ,-1);
		  nLengthW-=1;
	  }
	  ~CMYA2W()
	  {		
		  if(m_psz!=m_szBuffer)
			  free(m_psz);
	  }

	  operator LPWSTR()
	  {
		  return( m_psz );
	  }
	  int Len()const{return nLengthW;}

protected:
	void Init( _In_opt_ LPCSTR psz, _In_ UINT nConvertCodePage,int pszlen)
	{
		if(pszlen==0)
		{
			*m_psz=0;
			nLengthW=0;
			return;
		}
		nLengthW=_countof(m_szBuffer);
		int res=::MultiByteToWideChar( nConvertCodePage, 0, psz, pszlen, m_psz, nLengthW);
		if (res==0)
		{
			if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
			{
				nLengthW = ::MultiByteToWideChar( nConvertCodePage, 0, psz, pszlen, nullptr, 0);
				m_psz=(LPWSTR)malloc(nLengthW*sizeof(WCHAR));
				res=::MultiByteToWideChar( nConvertCodePage, 0, psz, pszlen, m_psz, nLengthW);
				if(res!=0)
					nLengthW=res;
			}			
		}
		else
			nLengthW=res;
	}

public:
	LPWSTR m_psz;
	WCHAR m_szBuffer[32];
	int nLengthW;
private:
	CMYA2W( const CMYA2W& );
	CMYA2W& operator=( const CMYA2W& );
};

class CMYA2WEX:public CMYA2W
{
public:
	CMYA2WEX(LPCSTR psz,int len)
	{
		Init( psz, CP_ACP,len);
	}
	CMYA2WEX(LPCSTR psz,int len, UINT nCodePage)
	{
		Init( psz, nCodePage,len);
	}
private:
	CMYA2WEX( const CMYA2WEX& );
	CMYA2WEX& operator=( const CMYA2WEX& );
};

template<class StringType>
StringType BufToStringTmpl(void* buf,size_t bufsz)
{
	const char list[]="0123456789ABCDEF";
	StringType temp;
	while(bufsz--)
	{
		temp.AppendChar( list[((*(UCHAR*)buf)>>4)&0xf] );
		temp.AppendChar( list[(*(UCHAR*)buf)&0xf] );
		buf=((UCHAR*)buf)+1;
	}
	return temp;
}
template<class BufType>
CAtlStringA BufToAtlStringA(const BufType* buf,size_t bufsz)
{
	return BufToStringTmpl<CAtlStringA>((void*)buf,bufsz);
}
template<class BufType>
CAtlStringW BufToAtlStringW(const BufType* buf,size_t bufsz)
{
	return BufToStringTmpl<CAtlStringW>((void*)buf,bufsz);
}
template<class BufType>
CStringA BufToStringA(const BufType* buf,size_t bufsz)
{
	return BufToStringTmpl<CStringA>((void*)buf,bufsz);
}
template<class BufType>
CStringW BufToStringW(const BufType* buf,size_t bufsz)
{
	return BufToStringTmpl<CStringW>((void*)buf,bufsz);
}
template<class StrType>
int StringToBufTmpl(void *buf,size_t bufsz,const StrType* str)
{
	const char list[]="0123456789ABCDEF";
	size_t pos=0;
	while(pos<bufsz)
	{
		char* wordh=StrChrIA((char*)list,*(str+pos*2));
		char* wordl=StrChrIA((char*)list,*(str+pos*2+1));
		if(wordh && wordl)
			*((UCHAR*)buf+pos)= ((wordh-list)<<4)|(wordl-list);
		else
			return -1;
		pos++;
	}
	return pos;
}
template<class BufType>
int StringToBuf(BufType *buf,size_t bufsz,LPCSTR str)
{
	return StringToBufTmpl(buf,bufsz,str);
}
template<class BufType,size_t sz>
int StringToBuf(BufType (&buf)[sz],LPCSTR str)
{
	return StringToBuf<BufType>(buf,sz*sizeof(BufType),str)/sizeof(BufType);
}
template<class BufType>
int StringToBuf(BufType *buf,size_t bufsz,LPCWSTR str)
{
	return StringToBufTmpl(buf,bufsz,str);
}
template<class BufType,size_t sz>
int StringToBuf(BufType (&buf)[sz],LPCWSTR str)
{
	return StringToBuf<BufType>(buf,sz*sizeof(BufType),str)/sizeof(BufType);
}

struct IBuffer:public IPtrBase<IBuffer,unsigned long>
{
	virtual char* GetBuffer()=0;
	virtual size_t GetLen()=0;
	virtual char* Alloc(DWORD size)=0;
};
typedef CIPtr<IBuffer> LPBUFFER;
class CNormalBuffer:public IBuffer
{
private:
	char *buffer;
	DWORD sz;
	CNormalBuffer():buffer(nullptr),sz(0){}
	~CNormalBuffer(){if(buffer) free(buffer);}
	
	char* GetBuffer(){return buffer;}
	size_t GetLen(){return sz;}
	char* Alloc(DWORD size)
	{
		DWORD allocsz=UPROUND(size,256);
		if(sz>=allocsz) return buffer;
		buffer=(char*)realloc(buffer,allocsz);
		sz=allocsz;
		return buffer;
	}
public:
	static LPBUFFER CreateBuffer(){ return new CNormalBuffer();}
};
class CVirtualBuffer:public IBuffer
{
private:
	char *buffer;
	size_t sz;
	CVirtualBuffer():buffer(nullptr),sz(0){}
	~CVirtualBuffer(){if(buffer) ::VirtualFree(buffer,sz,MEM_RELEASE);}
	
	char* GetBuffer(){return buffer;}
	size_t GetLen(){return sz;}
	char* Alloc(DWORD size)
	{
		buffer=(char*)VirtualAlloc(nullptr,size,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
		MEMORY_BASIC_INFORMATION info;
		::VirtualQuery(buffer,&info,sizeof(info));
		sz=info.RegionSize;
		::VirtualLock(buffer,sz);
		return buffer;
	}
public:
	static LPBUFFER CreateBuffer(){ return new CVirtualBuffer();}
};

template<class DATABLOCK>
class CDataBlockAllocor
{
private:
	struct DataBlockSec
	{
		DATABLOCK data;
		DataBlockSec *next;
	};
	DataBlockSec *head,*tail;
	DWORD pagesize;
	CCriticalLock lock;
#ifdef _DEBUG
	DWORD count;
#endif
public:
	CDataBlockAllocor()
	{
		head=tail=nullptr;
		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		pagesize=sysinfo.dwPageSize;
#ifdef _DEBUG
		count=0;
#endif
	}
	~CDataBlockAllocor()
	{
		ATLASSERT(count==0);
	}
	DATABLOCK *AllocBlock()
	{
		CAutoCLock al(lock);
		ATLASSERT( (head==nullptr && tail==nullptr) || (head && tail) );
		if(head==nullptr && tail==nullptr)
		{
			DataBlockSec *newbp=(DataBlockSec*)::VirtualAlloc(nullptr,pagesize,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
			::VirtualLock(newbp,pagesize);
			int count=pagesize/sizeof(DataBlockSec);
			head=newbp;
			for(int i=0;i<count-1;i++)
			{
				newbp->next=newbp+1;
				newbp++;
			}
			tail=newbp;
			newbp->next=nullptr;
		}
		DataBlockSec *resblock=head;
		head=head->next;
		if(head==nullptr)
			tail=nullptr;
#ifdef _DEBUG
		count++;
#endif
		return &(resblock->data);
	}
	void FreeBlock(DATABLOCK *p)
	{
		CAutoCLock al(lock);
		ATLASSERT( (head==nullptr && tail==nullptr) || (head && tail) );
		DataBlockSec* releasep=(DataBlockSec*)( ((char*)p)-(int)(&(((DataBlockSec*)nullptr)->data)) );
		releasep->next=nullptr;
		if(head==nullptr && tail==nullptr)
		{
			head=tail=releasep;
		}
		else
		{
			tail->next=releasep;
			tail=releasep;
		}
#ifdef _DEBUG
		count--;
#endif
	}
};

class CCycleList
{
public:
	class ListCellBase
	{
		friend class CCycleList;
	private:
		CCryptPoint<ListCellBase> next;
		CCryptPoint<ListCellBase> prev;
		size_t typesign;
	public:
		ListCellBase(size_t typesign);
		size_t GetTypeSign();
		ListCellBase *GetNext() const;
		ListCellBase *GetPrev() const;
		void Swap(ListCellBase* one);
		void ReplaceSelf(ListCellBase* one);
		void RemoveSelf();
		void InsertAfter(ListCellBase* element);
		void InsertBefore(ListCellBase* element);
		inline bool InLink()
		{
#ifdef _DEBUG
			ATLASSERT( (next==nullptr && prev==nullptr) || (next!=nullptr && prev!=nullptr) );
			return next!=nullptr && prev!=nullptr;
#else
			return true;
#endif
		}
		virtual ~ListCellBase();
	};
private:
	ListCellBase base;
public:
	CCycleList();
	size_t GetCount();
	void RemoveAll();
	~CCycleList();
	void Add(ListCellBase* cell);
	POSITION GetHeadPosition() const;
	ListCellBase* GetHead() const{return (ListCellBase*)GetHeadPosition();}
	ListCellBase* GetNext(POSITION &pos) const;
	POSITION GetTailPosition() const;
	ListCellBase* GetTail()const{return (ListCellBase*)GetTailPosition();}
	ListCellBase* GetPrev(POSITION &pos) const;
	void RemoveAt(POSITION pos);
	void RemoveAt(ListCellBase* element);
	void InsertAfter(POSITION pos,ListCellBase* element);
	void InsertBefore(POSITION pos,ListCellBase* element);
};


class CCmdLineParser
{
private:
	static const TCHAR m_sDelimeters[];
	static const TCHAR m_sQuotes[];
	static const TCHAR m_sValueSep[];
	CRBMap<CAtlString,CAtlString,CStringElementTraitsNoCase<CAtlString> > m_ValsMap;
#ifdef _DEBUG
	void OutputAll() const;
#endif
public:
	CCmdLineParser(){}
	CCmdLineParser(LPCTSTR sCmdLine);
	CCmdLineParser(int argc,_TCHAR* argv[]){ParseCmdLine(argc,argv);}
	bool HasKey(const CAtlString key) const;
	bool GetValue(const CAtlString key,CAtlString &value) const;
	CAtlString GetValue(const CAtlString key)const;
	int ParseCmdLine(LPCTSTR sCmdLine);
	int ParseCmdLine(int argc,_TCHAR* argv[]);
};

void CreatePathForFile(CAtlStringW file);

class CSockAddr
{
public:
	struct SAddr
	{
		sockaddrstore buf;
		int len;
		const sockaddrstore* addr()const{return (const sockaddrstore*)&buf;}
		SAddr():len(0){}
		SAddr(const SAddr&a)
		{
			len=a.len;
			if(len>0)
				memcpy_s(&buf,sizeof(buf),&a.buf,min(a.len,sizeof(buf)));
		}
	};
private:
	vector<SAddr> addrlist;
public:
	CSockAddr(){}
	CSockAddr(PCWSTR host,PCWSTR port,bool passive=false,int af=AF_UNSPEC)
	{
		Set(host,port,passive,af);
	}
	CSockAddr(CSockAddr&& a):addrlist(std::move(a.addrlist))
	{
	}
	inline const sockaddrstore* Addr() const
	{
		if(addrlist.empty()) return nullptr;
		return (const sockaddrstore*)&addrlist[0].buf;
	}
	inline int Len() const
	{
		if(addrlist.empty()) return 0;
		return addrlist[0].len;
	}
	inline int AF() const
	{
		if(addrlist.empty()) return 0;
		return addrlist[0].buf.sa_family;
	}
	inline const vector<SAddr>& AddrList() const {return addrlist;}
	const CSockAddr& Set(PCWSTR host,PCWSTR port,bool passive=false,int af=AF_UNSPEC);
	static bool GetAddrName(const SOCKADDR* pSockaddr,int SockaddrLength,CAtlStringW &name,CAtlStringW &port);
};

class CResourceStream:public IStream,public CComObjectRootEx<CComSingleThreadModel>
{
private:
	BEGIN_COM_MAP(CResourceStream)
		COM_INTERFACE_ENTRY(ISequentialStream)
		COM_INTERFACE_ENTRY(IStream)
	END_COM_MAP()

	DWORD size;
	BYTE* lpRsrc;
	DWORD readpos;
	CResourceStream();
	bool GetRes(HMODULE hModule,LPCWSTR lpName,LPCWSTR lpType);
	~CResourceStream();

	HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
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
public:
	static CComPtr<IStream> CreateInstanse(HMODULE hModule,LPCWSTR lpName,LPCWSTR lpType);
};

void debugstring(LPTSTR pszFormat, ...);
#pragma once
#pragma warning(disable:4996)
#include <atlbase.h>
#include <atlstr.h>
#include <vector>
#include <stack>
#include <windows.h>
#include <list>
#include <algorithm>
#include <map>
#include "Random32.h"
#pragma comment(lib,"dbghelp.lib")
#include <WtsApi32.h>
#pragma comment(lib,"WtsApi32.lib")
using namespace std;
#define UPROUND(n,base) (((n)+((base)-1))/(base)*(base))
#define GetChildOffset(ClassOrStruct,Child) ((int)(&((ClassOrStruct*)nullptr)->Child))
#ifdef _DEBUG
#include <crtdbg.h>
#endif

static const char* HttpTimeFormatA="%a, %d %b %Y %H:%M:%S GMT";
static const wchar_t* HttpTimeFormatW=L"%a, %d %b %Y %H:%M:%S GMT";

template<class TYPE>
class CCryptPoint
{
private:
	TYPE* p;
public:
	inline const CCryptPoint<TYPE>& operator=(const CCryptPoint<TYPE> &other)
	{
		p=other.p;
		return *this;
	}
	CCryptPoint(const CCryptPoint<TYPE> &other)
	{
		p=other.p;
	}
#ifndef _DEBUG
	CCryptPoint(const TYPE *ptr=nullptr)
	{
		p=(TYPE*)::EncodePointer((PVOID)ptr);
	}
	inline const TYPE *operator =(const TYPE *ptr)
	{
		p=(TYPE*)::EncodePointer((PVOID)ptr);
		return ptr;
	}
	inline operator TYPE*() const
	{
		return (TYPE*)::DecodePointer((PVOID)p);
	}
	inline TYPE* operator->() const
	{
		return (TYPE*)::DecodePointer((PVOID)p);
	}
#else
	CCryptPoint(const TYPE *ptr=nullptr)
	{
		p=const_cast<TYPE*>(ptr);
	}
	inline const TYPE *operator =(const TYPE *ptr)
	{
		p=const_cast<TYPE*>(ptr);
		return ptr;
	}
	inline operator TYPE*() const
	{
		return p;
	}
	inline TYPE* operator->() const
	{
		return p;
	}
#endif
};
struct sockaddrstore:public sockaddr
{
	char park[32-sizeof(sockaddr)];
};
class CCriticalLock
{
private:
	CRITICAL_SECTION section;
public:
	CCriticalLock()
	{
		::InitializeCriticalSectionAndSpinCount(&section,0x800);
	}
	~CCriticalLock()
	{
		DeleteCriticalSection(&section);
	}
	void Lock()
	{
		EnterCriticalSection(&section);
	}
	void Unlock()
	{
		LeaveCriticalSection(&section);
	}
	operator const LPCRITICAL_SECTION(){return &section;}
};
class CAutoCLock
{
private:
	CCriticalLock* pcs;
public:
	CAutoCLock(CCriticalLock &srpcs):pcs(&srpcs)
	{
		pcs->Lock();
	}
	~CAutoCLock()
	{
		pcs->Unlock();
	}
};
class CAutoCTryLock
{
private:
	CCriticalLock* pcs;
	BOOL Locked;
public:
	CAutoCTryLock(CCriticalLock &srpcs):pcs(&srpcs)
	{
		Locked=::TryEnterCriticalSection(*pcs);
	}
	~CAutoCTryLock()
	{
		if(Locked)
			pcs->Unlock();
	}
	BOOL IsLocked(){return Locked;}
	operator BOOL(){return Locked;}
};
class CExSemaphore
{
private:
	HANDLE m_handle;
public:
	CExSemaphore(LONG lInitialCount=0,LONG lMaximumCount=LONG_MAX,LPCTSTR lpName=nullptr,
		LPSECURITY_ATTRIBUTES lpSemaphoreAttributes=nullptr)
	{
		m_handle=CreateSemaphore(lpSemaphoreAttributes,lInitialCount,lMaximumCount,lpName);
		ATLASSERT(m_handle);
	}
	~CExSemaphore()
	{
		CloseHandle(m_handle);
	}
	BOOL Release(LONG count=1)
	{
		return ReleaseSemaphore(m_handle,count,nullptr);
	}
	BOOL Wait(DWORD wait_time=INFINITE)
	{
		return WAIT_OBJECT_0==WaitForSingleObject(m_handle,wait_time);
	}
};
class long_s
{
private:
	long i;
	CCriticalLock lock;
public:
	long_s(const long j)
	{
		CAutoCLock al(lock);
		i=(long)j;
	}
	long operator++(int)
	{
		CAutoCLock al(lock);
		return i++;
	}
	long operator--(int)
	{
		CAutoCLock al(lock);
		return i--;
	}
	long operator++()
	{
		CAutoCLock al(lock);
		return ++i;
	}
	long operator--()
	{
		CAutoCLock al(lock);
		return --i;
	}
	long operator=(const long j)
	{
		CAutoCLock al(lock);
		return i=j;
	}
	bool operator==(long j)
	{
		CAutoCLock al(lock);
		return i==j;
	}
	bool operator<(long j)
	{
		CAutoCLock al(lock);
		return i<j;
	}
	bool operator>(long j)
	{
		CAutoCLock al(lock);
		return i>j;
	}
	bool operator!=(long j)
	{
		return !(*this==j);
	}
	long Value(){return i;}
};
class CMyMutex
{
protected:
	HANDLE mu;
public:
	CMyMutex(LPCTSTR name=nullptr)
	{
		mu=::CreateMutex(0, FALSE, name);
	}
	~CMyMutex()
	{
		::CloseHandle(mu);
	}
	inline DWORD Lock(DWORD waittime=-1)
	{
		return ::WaitForSingleObject(mu,waittime);
	}
	inline BOOL Unlock(){return ::ReleaseMutex(mu);}
	operator const HANDLE(){return mu;}
};
class CAutoMutexLock
{
private:
	HANDLE mu;
public:
	CAutoMutexLock(const HANDLE mu):mu(mu)
	{
		::WaitForSingleObject(mu,INFINITE);
	}
	~CAutoMutexLock()
	{
		ReleaseMutex(mu);
	}
};

class CMyEvent
{
protected:
	HANDLE  hEvent;
public:
	CMyEvent(BOOL bManualReset=FALSE, BOOL bInitialState=FALSE, LPTSTR lpName=nullptr):hEvent(nullptr)
	{
		hEvent=::CreateEvent(nullptr,bManualReset,bInitialState,lpName);
	}
	CMyEvent(DWORD  dwDesiredAccess,BOOL  bInheritHandle,LPCTSTR  lpName):hEvent(nullptr)
	{
		hEvent=::OpenEvent(dwDesiredAccess,bInheritHandle,lpName );
	}
	~CMyEvent()
	{
		if(hEvent!=nullptr)
		{
			::CloseHandle(hEvent);
		}
	}
	inline operator HANDLE(){return hEvent;}
	inline BOOL  SetEvent()
	{
		return ::SetEvent(hEvent);
	}
	inline BOOL  PulseEvent()
	{
		return ::PulseEvent(hEvent);
	}
	inline BOOL  ResetEvent()
	{
		return ::ResetEvent(hEvent);
	}
	inline BOOL Wait(DWORD time=INFINITE)
	{
		return WAIT_TIMEOUT!=::WaitForSingleObject(hEvent,time);
	}
};
class ReadWriteLock
{
private:
	enum{LOCK_LEVEL_NONE,LOCK_LEVEL_WRITE,LOCK_LEVEL_READ};
	int m_currentLevel;//当前状态
	int    m_readCount;//读计数  
	CMyEvent m_unlockEvent;//锁切换等待信号，手动信号，状态切换受m_csStateChange保护
	CCriticalLock m_accessMutex;//lock函数进入保护，lock函数只能一个线程访问
	CCriticalLock m_csStateChange;//内部状态修改保护
	ReadWriteLock(const ReadWriteLock&){};
	ReadWriteLock& operator=(const ReadWriteLock&){return *this;}
public:
	ReadWriteLock():m_unlockEvent(TRUE, FALSE)
	{
		m_currentLevel = LOCK_LEVEL_NONE;
		m_readCount    = 0;
	}
	~ReadWriteLock()
	{
	}

	bool readlock(DWORD waittime=INFINITE)
	{
		CAutoCLock al(m_accessMutex);
		if(m_currentLevel==LOCK_LEVEL_WRITE)//不需要读取保护，即使判断时写解锁了，最多也是调用一下下面等待后立刻返回
		{
			if(!m_unlockEvent.Wait(waittime))
				return false;
		}
		CAutoCLock al2(m_csStateChange);
		m_currentLevel = LOCK_LEVEL_READ;
		m_readCount ++;
		m_unlockEvent.ResetEvent();
		return true;
	}
	bool writelock(DWORD waittime=INFINITE)
	{
		CAutoCLock al(m_accessMutex);
		if(m_currentLevel!=LOCK_LEVEL_NONE)//同上
		{
			if(!m_unlockEvent.Wait(waittime))
				return false;
		}
		CAutoCLock al2(m_csStateChange);
		m_currentLevel = LOCK_LEVEL_WRITE;
		m_unlockEvent.ResetEvent();
		return true;
	} // lock()

	bool readunlock()
	{
		CAutoCLock al2(m_csStateChange);
		if ( m_currentLevel != LOCK_LEVEL_READ )
			return false;
		m_readCount --;
		if ( m_readCount == 0 )
		{
			m_currentLevel = LOCK_LEVEL_NONE;
			m_unlockEvent.SetEvent();
		}
		return true;
	}
	bool writeunlock()
	{
		CAutoCLock al2(m_csStateChange);
		if ( m_currentLevel != LOCK_LEVEL_WRITE )
			return false;
		m_currentLevel = LOCK_LEVEL_NONE;
		m_unlockEvent.SetEvent();
		return true;
	}
};
class CAutoReadLock
{
private:
	ReadWriteLock *rwl;
public:
	CAutoReadLock(ReadWriteLock &lock):rwl(&lock)
	{
		rwl->readlock();
	}
	~CAutoReadLock()
	{
		ATLVERIFY(rwl->readunlock());
	}
};
class CAutoReadTryLock
{
private:
	ReadWriteLock *rwl;
	bool locked;
public:
	CAutoReadTryLock(ReadWriteLock &lock,DWORD waittime=0):rwl(&lock)
	{
		locked=rwl->readlock(waittime);
	}
	bool Locked() const {return locked;}
	~CAutoReadTryLock()
	{
		if(locked)
		{
			ATLVERIFY(rwl->readunlock());
		}
	}
};
class CAutoWriteLock
{
private:
	ReadWriteLock *rwl;
public:
	CAutoWriteLock(ReadWriteLock &lock):rwl(&lock)
	{
		rwl->writelock();
	}
	~CAutoWriteLock()
	{
		ATLVERIFY(rwl->writeunlock());
	}
};
class CAutoWriteTryLock
{
private:
	ReadWriteLock *rwl;
	bool locked;
public:
	CAutoWriteTryLock(ReadWriteLock &lock,DWORD waittime=0):rwl(&lock)
	{
		locked=rwl->writelock(waittime);
	}
	bool Locked() const {return locked;}
	~CAutoWriteTryLock()
	{
		if(locked)
		{
			ATLVERIFY(rwl->writeunlock());
		}
	}
};


template<class TYPE,int maxsize=10>
class CCommunicatQueue
{
protected:
	list<TYPE> c;
	CCriticalLock lock;
public:
	bool Push(const TYPE &one)
	{
		if(c.size()>maxsize) return false;
		CAutoCLock al(lock);
		c.push_back(one);
		return true;
	}
	bool Pop(TYPE &one)
	{
		CAutoCLock al(lock);
		if(c.size()==0) return false;
		one=c.front();
		c.pop_front();
		return true;
	}
	bool IsThisInQueue(const TYPE &test)
	{
		CAutoCLock al(lock);
		for(list<TYPE>::iterator i=c.begin();i!=c.end();i++)
		{
			if(*i==test)
				return true;
		}
		return false;
	}
	size_t Size(){return c.size();}
	bool IsFull(){return c.size()>=maxsize;}
	bool Empty(){return c.empty();}
};
template<class TYPE,unsigned size=100>
class CCommunicatQueue_ForP2P
{
protected:
	TYPE link[size];
	size_t top;
	size_t buttom;
public:
	CCommunicatQueue_ForP2P():top(0),buttom(0)
	{}
	bool Push(const TYPE &one)
	{
		if(top==(size-1))
		{
			top=0;
			if(top==buttom)
			{
				top=(size-1);
				return false;
			}
		}
		else
		{
			top+=1;
			if(top==buttom)
			{
				top-=1;
				return false;
			}
		}

		//::new(top) TYPE();
		link[top]=one;
		return true;
	}
	bool Pop(TYPE &one)
	{
		if(top==buttom)
		{
			return false;
		}

		if(buttom == (size-1))
			buttom=0;
		else
			buttom+=1;
		one=link[buttom];
		//buttom->~TYPE();
		return true;
	}
};

class FileStream : public IStream
{
private:
    FileStream(HANDLE hFile);
    ~FileStream();
	CAtlStringW filename;
public:
	static HRESULT OpenFile(LPCWSTR pName, IStream ** ppStream,bool write=false);
	static HRESULT CreateNewFile(LPCWSTR pName, IStream ** ppStream);
	static HRESULT CreateFile( LPCWSTR lpFileName,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes,
		HANDLE hTemplateFile,
		IStream ** ppStream);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

    // ISequentialStream Interface
public:
	HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);
    // IStream Interface
public:
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size);
    HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*,
        ULARGE_INTEGER*);
	HRESULT STDMETHODCALLTYPE Commit(DWORD);
	HRESULT STDMETHODCALLTYPE Revert(void);
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
	HRESULT STDMETHODCALLTYPE Clone(IStream **);
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
        ULARGE_INTEGER* lpNewFilePointer);
	HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);
private:
    HANDLE _hFile;
    LONG _refcount;
};

class InterlockCounter
{
private:
	long m;
public:
	InterlockCounter():m(0)
	{
	}
	InterlockCounter(const long b)
	{
		::InterlockedExchange(&m,b);
	}
	operator unsigned long(){return m;}
	long operator++()
	{
		return InterlockedIncrement(&m);
	}
	long operator--()
	{
		return InterlockedDecrement(&m);
	}
	long operator++(int)
	{
		return ::InterlockedExchangeAdd(&m,1);
	}
	long operator--(int)
	{
		return ::InterlockedExchangeAdd(&m,-1);
	}
	long operator=(const long b)
	{
		return ::InterlockedExchange(&m,b);
	}
};

template<class TYPE,class CounterType=InterlockCounter>
class IPtrBase
{
private:
	CounterType m_ref;
public:
	const CounterType& GetRefCount() const{return m_ref;}
	IPtrBase():m_ref(0){};
	long AddRef()
	{
		return ++m_ref;
	}
	long Release()
	{
		long res=--m_ref;
		if(res==0)
		{
			delete static_cast<TYPE*>(this);
		}
		return res;
	}
};
template<class TYPE>
struct IPtrCast
{
	template<class OtherType>
	inline const TYPE* Cast(const OtherType* far_p) const
	{
		return dynamic_cast<const TYPE*>(far_p);
	}
};
template<class TYPE,class CastPtr=IPtrCast<TYPE> >
class CIPtr
{
private:
	TYPE* point;
public:
	class _NoAddRefReleaseOnCIPtr : public TYPE
	{
	private:
		long AddRef()
		{
			ATLASSERT(false);
			//do not call this
			return 0;
		}
		long Release()
		{
			ATLASSERT(false);
			//do not call this
			return 0;
		}
	};
	inline const TYPE* GetPoint()const
	{return point;}
	CIPtr(const TYPE *p)
	{
		point=const_cast<TYPE*>(p);
		if(point!=nullptr)
			point->AddRef();
	}
	~CIPtr()
	{
		if(point!=nullptr)
			point->Release();
	}
	CIPtr():point(nullptr)
	{
	}
	CIPtr(const CIPtr<TYPE,CastPtr> &cip)
	{
		point=cip.point;
		if(point!=nullptr)
			point->AddRef();
	}
	CIPtr(CIPtr<TYPE,CastPtr> &&cip)
	{
		point=cip.point;
		cip.point=nullptr;
	}
	template<class OtherType,class OtherCastPtr>
	CIPtr(const CIPtr<OtherType,OtherCastPtr> &cip):point(nullptr)
	{
		TypeCast(cip.GetPoint());
	}
	template<class OtherType>
	CIPtr(const OtherType *p):point(nullptr)
	{
		TypeCast(p);
	}
protected:
	template<class OtherType>
	inline TYPE* TypeCast(const OtherType* far_p)
	{
		CastPtr ptrcast;
		const TYPE* temp=ptrcast.Cast(far_p);
		return (*this)=temp;
	}
public:
	template<class OtherType>
	inline _NoAddRefReleaseOnCIPtr* operator=(const OtherType *farp)
	{
		return (_NoAddRefReleaseOnCIPtr*)TypeCast(farp);
	}
	template<class OtherType,class OtherCastPtr>
	inline _NoAddRefReleaseOnCIPtr* operator=(const CIPtr<OtherType,OtherCastPtr> &cip)
	{
		return (_NoAddRefReleaseOnCIPtr*)TypeCast(cip.GetPoint());
	}
	inline _NoAddRefReleaseOnCIPtr* operator=(const CIPtr<TYPE,CastPtr> &cip)
	{
		if(point!=cip.point)
		{
			if(point!=nullptr)
				point->Release();
			point=const_cast<TYPE*>(cip.GetPoint());
			if(point!=nullptr)
				point->AddRef();
		}
		return (_NoAddRefReleaseOnCIPtr*)(TYPE*)point;
	}
	inline _NoAddRefReleaseOnCIPtr* operator=(const TYPE *farp)
	{
		if(point!=farp)
		{
			if(point!=nullptr)
				point->Release();
			point=const_cast<TYPE*>(farp);
			if(point!=nullptr)
				point->AddRef();
		}
		return (_NoAddRefReleaseOnCIPtr*)(TYPE*)point;
	}
	inline operator TYPE*() const
	{
		return point;
	}
	inline _NoAddRefReleaseOnCIPtr* operator->() const
	{
		ATLASSERT(point!=nullptr);
		return (_NoAddRefReleaseOnCIPtr*)(TYPE*)point;
	}
}; 

template<class Type>
class RandomSort
{
private:
	struct randstu
	{
		unsigned long randint;
		Type p;
	};
	vector<randstu> randvector;
	static bool less_randstu(randstu &a,randstu &b)
	{
		return a.randint<b.randint;
	}
	CRandom32 rander;
public:
	void clear()
	{
		randvector.clear();
	}
	void insert(Type p)
	{
		randstu one;
		one.randint=rander.Next();
		one.p=p;
		randvector.push_back(one);
	}
	void sort()
	{
		if(randvector.empty()) return;
		::sort(randvector.begin(),randvector.end(),less_randstu);
	}
	size_t count(){return randvector.size();}
	Type operator[](size_t index)
	{
		return randvector[index].p;
	}
};


class ExApis
{
private:
	typedef int (__stdcall *MessageBoxTimeoutAApi)(IN HWND hWnd,IN LPCSTR lpText,IN LPCSTR lpCaption,
		IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	typedef int (__stdcall *MessageBoxTimeoutWApi)(IN HWND hWnd,IN LPCWSTR lpText, IN LPCWSTR lpCaption,
		IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	typedef BOOL  (WINAPI  *DnsFlushResolverCacheApi)(VOID);
	MessageBoxTimeoutAApi pMessageBoxTimeoutA;
	MessageBoxTimeoutWApi pMessageBoxTimeoutW;
	DnsFlushResolverCacheApi pDnsFlushResolverCache;

	HMODULE dnsapiModule;
public:
	ExApis():pMessageBoxTimeoutA(0),pMessageBoxTimeoutW(0),pDnsFlushResolverCache(0),dnsapiModule(nullptr)
	{
	}
public:
	int MessageBoxTimeoutA(IN HWND hWnd,IN LPCSTR lpText,IN LPCSTR lpCaption,
		IN UINT uType, IN WORD wLanguageId=0, IN DWORD dwMilliseconds=1000)
	{
		if(pMessageBoxTimeoutA==nullptr)
		{
			HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
			if (hUser32)
			{            
				pMessageBoxTimeoutA = (MessageBoxTimeoutAApi)GetProcAddress(hUser32,"MessageBoxTimeoutA");
			}
		}
		if(pMessageBoxTimeoutA) return pMessageBoxTimeoutA(hWnd,lpText,lpCaption,uType,wLanguageId,dwMilliseconds);
		else return 0;
	}
	int MessageBoxTimeoutW(IN HWND hWnd,IN LPCWSTR lpText, IN LPCWSTR lpCaption,
		IN UINT uType, IN WORD wLanguageId=0, IN DWORD dwMilliseconds=1000)
	{
		if(pMessageBoxTimeoutW==nullptr)
		{
			HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
			if (hUser32)
			{            
				pMessageBoxTimeoutW = (MessageBoxTimeoutWApi)GetProcAddress(hUser32,"MessageBoxTimeoutW");
			}
		}
		if(pMessageBoxTimeoutW) return pMessageBoxTimeoutW(hWnd,lpText,lpCaption,uType,wLanguageId,dwMilliseconds);
		else return 0;
	}
	int MessageBoxTimeout(IN HWND hWnd,IN LPCTSTR lpText, IN LPCTSTR lpCaption,
		IN UINT uType, IN WORD wLanguageId=0, IN DWORD dwMilliseconds=1000)
	{
#ifdef UNICODE
		return MessageBoxTimeoutW(hWnd,lpText,lpCaption,uType,wLanguageId,dwMilliseconds);
#else
		return MessageBoxTimeoutA(hWnd,lpText,lpCaption,uType,wLanguageId,dwMilliseconds);
#endif
	}
	BOOL DnsFlushResolverCache()
	{
		if(pDnsFlushResolverCache==nullptr)
		{
			dnsapiModule=GetModuleHandle(_T("dnsapi.dll"));
			if(dnsapiModule==nullptr)
				dnsapiModule = ::LoadLibrary(_T("dnsapi.dll"));
			if(dnsapiModule)
			{
				pDnsFlushResolverCache=(DnsFlushResolverCacheApi) GetProcAddress (dnsapiModule,"DnsFlushResolverCache");
			}
		}
		if(pDnsFlushResolverCache) return pDnsFlushResolverCache();
		return 0;
	}
};

struct StrLess
{
	bool operator()(const CAtlStringA &a,const CAtlStringA &b) const
	{
		return a.Compare(b)<0;
	}
	bool operator()(const CAtlStringW &a,const CAtlStringW &b) const
	{
		return a.Compare(b)<0;
	}
	bool operator()(LPCSTR a,LPCSTR b) const
	{
		return strcmp(a,b)<0;
	}
	bool operator()(LPCWSTR a,LPCWSTR b) const
	{
		return wcscmp(a,b)<0;
	}
};

struct CString_Compare
{	// traits class for hash containers
	enum
	{	// parameters for hash table
		bucket_size = 4,	// 0 < bucket_size
		min_buckets = 8	// min_buckets = 2 ^^ N, 0 < N
	};
	StrLess strless;
	size_t operator()(const CAtlStringA &str) const
	{
		return real_hash((LPCSTR)str);
	}
	size_t operator()(const CAtlStringW &str) const
	{
		return real_hash((LPCWSTR)str);
	}
	size_t operator()(LPCSTR str) const
	{
		return real_hash(str);
	}
	size_t operator()(LPCWSTR str) const
	{
		return real_hash(str);
	}
	template<class TYPE>
	size_t real_hash(const TYPE * str) const
	{
		register size_t nr=0;
		register const TYPE* i=str;
		register TYPE word;
		while ((word=*i)!=0) 
		{ 
			nr*=16777619; 
			nr^=(UINT)word;
			i++;
		} 
		return((size_t) nr);

		/*
		register unsigned long __h = 0;
		register const TYPE* i=str;
		register TYPE word;
		while( (word=*i)!=0 )
		{
			__h = 5*__h + word;
			i++;
		}
		return size_t(__h);*/
	}
	template<class Param> 
	bool operator()(const Param &_Keyval1, 
		const Param &_Keyval2) const
	{	// test if _Keyval1 ordered before _Keyval2
		return strless(_Keyval1,_Keyval2);
	}
};
struct StrLessNoCase
{
	bool operator()(const CAtlStringA &a,const CAtlStringA &b) const
	{
		return a.CompareNoCase(b)<0;
	}
	bool operator()(const CAtlStringW &a,const CAtlStringW &b) const
	{
		return a.CompareNoCase(b)<0;
	}
	bool operator()(LPCSTR a,LPCSTR b) const
	{
		return _stricmp(a,b)<0;
	}
	bool operator()(LPCWSTR a,LPCWSTR b) const
	{
		return _wcsicmp(a,b)<0;
	}
};

struct CString_Compare_NoCase
{	// traits class for hash containers
	enum
	{	// parameters for hash table
		bucket_size = 4,	// 0 < bucket_size
		min_buckets = 8	// min_buckets = 2 ^^ N, 0 < N
	};
	StrLessNoCase strless;
	size_t operator()(const CAtlStringA &str) const
	{
		return real_hash((LPCSTR)str);
	}
	size_t operator()(const CAtlStringW &str) const
	{
		return real_hash((LPCWSTR)str);
	}
	size_t operator()(LPCSTR str) const
	{
		return real_hash(str);
	}
	size_t operator()(LPCWSTR str) const
	{
		return real_hash(str);
	}
	template<class TYPE>
	size_t real_hash(const TYPE * str) const
	{
		register size_t nr=0;
		register const TYPE* i=str;
		register TYPE word;
		while ((word=*i)!=0) 
		{ 
			if(word>='A' && word<='Z')
				word=word-(TYPE)('A'-'a');
			nr*=16777619; 
			nr^=(UINT)word;
			i++;
		}
		return((size_t) nr);
		/*
		register unsigned long __h = 0;
		register const TYPE * i=str;
		register TYPE word;
		while( (word=*i)!=0 )
		{
			if(word>='A' && word<='Z')
				word-=('A'-'a');
			__h = 5*__h + word;
			i++;
		}
		return size_t(__h);*/
	}
	template<class Param> 
	bool operator()(const Param &_Keyval1, 
		const Param &_Keyval2) const
	{	// test if _Keyval1 ordered before _Keyval2
		return strless(_Keyval1,_Keyval2);
	}
};

template< typename T >
class CStringElementTraitsNoCase
{
public:
	typedef typename T::PCXSTR INARGTYPE;
	typedef T& OUTARGTYPE;

	static void __cdecl CopyElements( _Out_capcount_(nElements) T* pDest, _In_count_(nElements) const T* pSrc, _In_ size_t nElements )
	{
		for( size_t iElement = 0; iElement < nElements; iElement++ )
		{
			pDest[iElement] = pSrc[iElement];
		}
	}

	static void __cdecl RelocateElements( _Out_capcount_(nElements) T* pDest, _In_count_(nElements) T* pSrc, _In_ size_t nElements )
	{
		Checked::memmove_s( pDest, nElements*sizeof( T ), pSrc, nElements*sizeof( T ) );
	}

	static ULONG __cdecl Hash( _In_ INARGTYPE str )
	{
		ATLENSURE( str != nullptr );
		register ULONG nHash = 0;
		register const T::XCHAR* pch = str;
		register T::XCHAR value;
		while( (value=*pch) != 0 )
		{
			if(value>='A' && value<='Z')
				value=value-(T::XCHAR)('A'-'a');
			nHash = (nHash<<5)+nHash+(value);
			pch++;
		}

		return( nHash );
	}

	static bool __cdecl CompareElements( _In_ INARGTYPE str1, _In_ INARGTYPE str2 )
	{
		return( T::StrTraits::StringCompareIgnore( str1, str2 ) == 0 );
	}

	static int __cdecl CompareElementsOrdered( _In_ INARGTYPE str1, _In_ INARGTYPE str2 )
	{
		return( T::StrTraits::StringCompareIgnore( str1, str2 ) );
	}
};

class NoLimitSecurityDescriptor
{
private:
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;
public:
	NoLimitSecurityDescriptor()
	{
		InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd,TRUE,nullptr,FALSE);
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = &sd;
	}
	operator LPSECURITY_ATTRIBUTES(){return &sa;}
};
class CShareMemory
{
protected:
	HANDLE memfile;
	DWORD filesize;
	LPVOID p;
public:
	CShareMemory(LPCWSTR name,DWORD size,BOOL shareWrite=false);
	~CShareMemory(void);
	void* GetBuffer();
};

class ToolCollection
{
public:
	static BOOL EnableDebugPrivilege(BOOL fEnable);
	static bool RegisterURLProtocal(const CAtlStringW protocalname,const CAtlStringW cmdline=L"-url \"%1\"");
	static bool RegisterAutoStart(const CAtlStringW keyname,const CAtlStringW cmdline);
	static bool RemoveAutoStart(const CAtlStringW keyname);
	static void AddSelfToWindowsFirewall(const CAtlStringW name);
};
class DebugTools
{
public:
	static void UseMiniDmpFile();
	static void UsePurecallHandler();
	static void UseUnhandledExceptionFilter();
private:
	static LONG WINAPI SelfUnhandledException(struct _EXCEPTION_POINTERS *ExceptionInfo);
	static LONG WINAPI SelfUnhandledExceptionFilter( __in  struct _EXCEPTION_POINTERS *ExceptionInfo);
	static void SelfPurecallHandler();
};
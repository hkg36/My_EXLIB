#include "extools.h"
#include <Ws2tcpip.h>

CRunOnlyOne::CRunOnlyOne(LPCTSTR mutexname,bool auto_exit_when_not_first)
{
	handle=::CreateMutex(NoLimitSecurityDescriptor(),FALSE,mutexname);
	if(handle==nullptr) exit(0);
	DWORD error=::GetLastError();
	if(error==ERROR_ALREADY_EXISTS || error==ERROR_ACCESS_DENIED)
		first=false;
	else
		first=true;
	if(auto_exit_when_not_first && first==false)
		exit(0);
}
bool CRunOnlyOne::IsFirst(){return first;}
CRunOnlyOne::~CRunOnlyOne()
{
	if(handle)
		CloseHandle(handle);
}

HRESULT CreateComObject(LPCWSTR file,
	const CLSID &clsid,IUnknown *pUnknownOuter,DWORD dwClsContext,const IID &iid,void **ppv,bool CoCreateFirst)
{
	if(CoCreateFirst)
	{
		if(S_OK==::CoCreateInstance(clsid,pUnknownOuter,dwClsContext,iid,ppv))
			return S_OK;
	}
	HMODULE lib=GetModuleHandle(file);
	if(lib==nullptr)
		lib=::LoadLibraryW(file);
	if(lib==nullptr)
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
	typedef HRESULT (WINAPI *pfDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
	pfDllGetClassObject getclassobj=nullptr;
	getclassobj=(pfDllGetClassObject)GetProcAddress(lib,"DllGetClassObject");
	CComPtr<IClassFactory> icf;
	if(S_OK==getclassobj(clsid,__uuidof(IClassFactory),(void**)&icf))
	{
		return icf->CreateInstance(pUnknownOuter,iid,ppv);
	}
	else if(!CoCreateFirst)
	{
		if(S_OK==::CoCreateInstance(clsid,pUnknownOuter,dwClsContext,iid,ppv))
			return S_OK;
	}
	return E_FAIL;
}



CCycleList::ListCellBase::ListCellBase(size_t typesign):typesign(typesign),next(nullptr),prev(nullptr){ATLASSERT(typesign!=0);}
size_t CCycleList::ListCellBase::GetTypeSign(){return typesign;}
CCycleList::ListCellBase *CCycleList::ListCellBase::GetNext() const
{
	if(next==nullptr || next->typesign==0) return nullptr;
	else return next;
}
CCycleList::ListCellBase *CCycleList::ListCellBase::GetPrev() const
{
	if(prev==nullptr || prev->typesign==0) return nullptr;
	else return prev;
}
void CCycleList::ListCellBase::Swap(CCycleList::ListCellBase* one)
{
	ATLASSERT(one);
	ATLASSERT(one->typesign!=0);
	ATLASSERT(typesign!=0);
	ATLASSERT(InLink());
	if(one->next==this)
	{
		ATLASSERT(this->prev==one);
		ListCellBase *allprev=one->prev;
		ListCellBase *allnext=this->next;

		allprev->next=this;
		this->prev=allprev;

		one->next=allnext;
		allnext->prev=one;

		this->next=one;
		one->prev=this;
	}
	else if(one->prev==this)
	{
		ATLASSERT(this->next==one);
		ListCellBase *allprev=prev;
		ListCellBase *allnext=one->next;

		allprev->next=one;
		one->prev=allprev;

		this->next=allnext;
		allnext->prev=this;

		one->next=this;
		this->prev=one;
	}
	else
	{
		ListCellBase *temp;
		this->prev->next=one;
		this->next->prev=one;
		one->prev->next=this;
		one->next->prev=this;

		temp=this->next;
		this->next=one->next;
		one->next=temp;

		temp=this->prev;
		this->prev=one->prev;
		one->prev=temp;
	}
}
void CCycleList::ListCellBase::ReplaceSelf(CCycleList::ListCellBase* one)
{
	ATLASSERT(one && one->prev==nullptr && one->next==nullptr);
	ATLASSERT(InLink());
	prev->next=one;
	one->prev=prev;
	next->prev=one;
	one->next=next;
	prev=nullptr;
	next=nullptr;
}
void CCycleList::ListCellBase::RemoveSelf()
{
	if(next==nullptr && prev==nullptr)return;
	prev->next=next;
	next->prev=prev;
	next=nullptr;
	prev=nullptr;
}
CCycleList::ListCellBase::~ListCellBase()
{
	RemoveSelf();
}
CCycleList::CCycleList():base(1)
{
	base.typesign=0;
	base.prev=&base;
	base.next=&base;
}
size_t CCycleList::GetCount()
{
	ListCellBase* p=base.next;
	size_t count=0;
	while(p!=&base)
	{
		count++;
		p=p->next;
	}
	return count;
}
void CCycleList::RemoveAll()
{
	ListCellBase* point=base.next;
	while(point!=&base)
	{
		ListCellBase* pnext=point->next;
		delete point;
		point=pnext;
	}
	base.prev=&base;
	base.next=&base;
}
CCycleList::~CCycleList()
{
	RemoveAll();
}
void CCycleList::Add(ListCellBase* cell)
{
	ATLASSERT(cell->next==nullptr && cell->prev==nullptr);
	ListCellBase* insertpoint=base.prev;
	base.prev=cell;
	cell->prev=insertpoint;
	cell->next=&base;
	insertpoint->next=cell;
}
POSITION CCycleList::GetHeadPosition() const
{
	if(base.next==&base) return nullptr;
	return (POSITION)(ListCellBase*)base.next;
}
CCycleList::ListCellBase* CCycleList::GetNext(POSITION &pos) const
{
	if(pos==nullptr) return nullptr;
	ListCellBase* nowpos=(ListCellBase*)pos;
	if(nowpos->next==&base) pos=nullptr;
	else pos=(POSITION)(ListCellBase*)nowpos->next;
	return nowpos;
}
POSITION CCycleList::GetTailPosition() const
{
	if(base.prev==&base) return nullptr;
	return (POSITION)(ListCellBase*) base.prev;
}
CCycleList::ListCellBase* CCycleList::GetPrev(POSITION &pos) const
{
	if(pos==nullptr) return nullptr;
	ListCellBase* nowpos=(ListCellBase*)pos;
	if(nowpos->prev==&base) pos=nullptr;
	else pos=(POSITION)(ListCellBase*)nowpos->prev;
	return nowpos;
}
void CCycleList::RemoveAt(POSITION pos)
{
	RemoveAt((ListCellBase*)pos);
}
void CCycleList::RemoveAt(ListCellBase* element)
{
#ifdef _DEBUG
	ListCellBase* checkpoint=element->next;
	bool checkok=false;
	while(checkpoint!=element)
	{
		if(checkpoint==&base)
		{
			checkok=true;
			break;
		}
		checkpoint=checkpoint->next;
	}
	ATLASSERT(checkok);//element must in this link;
#endif
	ListCellBase* eleprev=element->prev;
	ListCellBase* elenext=element->next;
	delete element;
	eleprev->next=elenext;
	elenext->prev=eleprev;
}
void CCycleList::InsertAfter(POSITION pos,ListCellBase* element)
{
	ListCellBase* nowpos=(ListCellBase*)pos;
	nowpos->InsertAfter(element);
}
void CCycleList::InsertBefore(POSITION pos,ListCellBase* element)
{
	ListCellBase* nowpos=(ListCellBase*)pos;
	nowpos->InsertBefore(element);
}
void CCycleList::ListCellBase::InsertAfter(ListCellBase* element)
{
	ATLASSERT(InLink());
	ListCellBase* nextpos=this->next;

	this->next=element;
	element->prev=this;

	nextpos->prev=element;
	element->next=nextpos;
}
void CCycleList::ListCellBase::InsertBefore(ListCellBase* element)
{
	ATLASSERT(InLink());
	ListCellBase* prevpos=this->prev;

	this->prev=element;
	element->next=this;

	prevpos->next=element;
	element->prev=prevpos;
}

const TCHAR CCmdLineParser::m_sDelimeters[] = _T("-/");//键的起始符
const TCHAR CCmdLineParser::m_sQuotes[] = _T("\"");    // Can be _T("\"\'"),  for instance
const TCHAR CCmdLineParser::m_sValueSep[] = _T(" :="); // Space MUST be in set
#ifdef _DEBUG
void CCmdLineParser::OutputAll() const
{
	ATLTRACE(_T("==OUTPUT COMMAND LINE==\r\n"));
	POSITION pos=m_ValsMap.GetHeadPosition();
	while(pos)
	{
		auto pair=m_ValsMap.GetNext(pos);
		ATLTRACE(_T("%s = %s \r\n"),pair->m_key,pair->m_value);
	}
	ATLTRACE(_T("==OUTPUT COMMAND LINE==\r\n"));
}
#endif
CCmdLineParser::CCmdLineParser(LPCTSTR sCmdLine)
{
	ParseCmdLine(sCmdLine);
}
bool CCmdLineParser::HasKey(const CAtlString key) const
{
	auto pair = m_ValsMap.Lookup(key);
	return pair!=nullptr;
}
bool CCmdLineParser::GetValue(const CAtlString key,CAtlString &value) const
{
	return m_ValsMap.Lookup(key,value);
}
CAtlString CCmdLineParser::GetValue(const CAtlString key) const
{
	CAtlString value;
	m_ValsMap.Lookup(key,value);
	return value;
}
int CCmdLineParser::ParseCmdLine(LPCTSTR sCmdLine) 
{
	if(!sCmdLine) return 0;
	m_ValsMap.RemoveAll();
	const CAtlString sEmpty;
	int nArgs = 0;
	LPCTSTR sCurrent = sCmdLine;
	LPCTSTR sEnd=sCmdLine+_tcslen(sCurrent);
	while(true) {
		// /Key:"arg"
		if(sCurrent>=sEnd) { break; } // No data left
		LPCTSTR sArg = _tcspbrk(sCurrent, m_sDelimeters);
		if(!sArg) break; // No delimeters found
		sArg =  _tcsinc(sArg);
		// Key:"arg"
		if(sArg>=sEnd) break; // String ends with delimeter
		LPCTSTR sVal = _tcspbrk(sArg, m_sValueSep);
		if(sVal == nullptr) { //Key ends command line
			CAtlString csKey(sArg);

			m_ValsMap.SetAt(csKey,sEmpty);
			break;
		} else if(sVal[0] == _T(' ') || _tcslen(sVal) == 1 ) { // Key with no value or cmdline ends with /Key:
			CAtlString csKey(sArg, sVal - sArg);
			if(!csKey.IsEmpty()) {
				m_ValsMap.SetAt(csKey, sEmpty);
			}
			sCurrent = _tcsinc(sVal);
			continue;
		} else { // Key with value
			CAtlString csKey(sArg, sVal - sArg);
			sVal = _tcsinc(sVal);
			// "arg"
			LPCTSTR sQuote = _tcspbrk(sVal, m_sQuotes), sEndQuote(nullptr);
			if(sQuote == sVal) { // Quoted String
				sQuote = _tcsinc(sVal);
				sEndQuote = _tcspbrk(sQuote, m_sQuotes);
			} else {
				sQuote = sVal;
				sEndQuote = _tcschr(sQuote, _T(' '));
			}

			if(sEndQuote == nullptr) { // No end quotes or terminating space, take rest of string
				CAtlString csVal(sQuote);
				if(!csKey.IsEmpty()) { // Prevent /:val case
					m_ValsMap.SetAt(csKey, csVal);//保存
				}
				break;
			} else { // End quote or space present
				if(!csKey.IsEmpty()) {    // Prevent /:"val" case
					CAtlString csVal(sQuote, sEndQuote - sQuote);
					m_ValsMap.SetAt(csKey, csVal);
				}
				sCurrent = _tcsinc(sEndQuote);
				continue;
			}
		}

	}
#ifdef _DEBUG
	OutputAll();
#endif
	return nArgs;
}
int CCmdLineParser::ParseCmdLine(int argc, _TCHAR* argv[])
{
	if(argc<=1) return 0;
	m_ValsMap.RemoveAll();
	for(int i=1;i<argc;i++)
	{
		LPCTSTR sArg = wcschr(m_sDelimeters,argv[i][0]);
		if(sArg)
		{
			LPCTSTR sVal=_tcspbrk(argv[i],m_sValueSep);
			if(sVal)
			{
				m_ValsMap.SetAt(CStringW(argv[i]+1,sVal-argv[i]-1),CStringW(sVal+1));
			}
			else
			{
				m_ValsMap.SetAt(CStringW(argv[i]+1),CStringW());
			}
		}
	}
#ifdef _DEBUG
	OutputAll();
#endif
	return m_ValsMap.GetCount();
}
void CreatePathForFile(CAtlStringW file)
{
	size_t index=file.Find('\\',0);
	DWORD res;
	while(index!=-1)
	{
		CStringW tmp=file.Left(index);
		if(::PathFileExistsW(tmp)==FALSE)
		{
			if(!::CreateDirectory(tmp,nullptr))
			{
				res=GetLastError();
			}
		}
		index+=1;
		index=file.Find('\\',index);
	}
}


const CSockAddr& CSockAddr::Set(PCWSTR host,PCWSTR port,bool passive,int af)
{
	ADDRINFOW hint,*addrres=nullptr;
	ZeroMemory(&hint,sizeof(hint));
	hint.ai_family=af;
	if(passive)
		hint.ai_flags=AI_PASSIVE;
	if(GetAddrInfoW(host,port,&hint,&addrres)==0)
	{
		ADDRINFOW *addrs=addrres;
		addrlist.clear();
		do
		{
			SAddr addr;
			addr.len=addrs->ai_addrlen;
			memcpy_s(&addr.buf,sizeof(addr.buf),addrs->ai_addr,addrs->ai_addrlen);
			addrlist.push_back(addr);
		}while(addrs=addrs->ai_next);
		FreeAddrInfoW(addrres);
	}
	return *this;
}
bool CSockAddr::GetAddrName(const SOCKADDR* pSockaddr,int SockaddrLength,CAtlStringW &name,CAtlStringW &port)
{
	bool res=0==GetNameInfoW(pSockaddr,SockaddrLength,
		name.GetBuffer(NI_MAXHOST),NI_MAXHOST,
		port.GetBuffer(NI_MAXSERV),NI_MAXSERV,
		NI_NUMERICHOST|NI_NUMERICSERV);
	name.ReleaseBuffer();
	port.ReleaseBuffer();
	return res;
}


CResourceStream::CResourceStream():lpRsrc(nullptr),readpos(0)
{
}
bool CResourceStream::GetRes(HMODULE hModule,LPCWSTR lpName,LPCWSTR lpType)
{
	HRSRC rsrc=::FindResource(hModule,lpName,lpType);
	if(rsrc==nullptr) return false;
	size=SizeofResource(hModule,rsrc);
	if(size==0) return false;
	HGLOBAL hg=LoadResource(hModule, rsrc);
	if(hg==nullptr) return false;
	lpRsrc = (BYTE*)LockResource(hg);
	return true;
}
CResourceStream::~CResourceStream()
{
}

HRESULT STDMETHODCALLTYPE CResourceStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	ULONG toread=min(cb,size-readpos);
	if(toread==0)
	{
		if(pcbRead)*pcbRead=0;
		return S_OK;
	}
	memcpy_s(pv,cb,lpRsrc+readpos,toread);
	readpos+=toread;
	if(pcbRead)*pcbRead=toread;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CResourceStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::SetSize(ULARGE_INTEGER size){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::CopyTo(IStream* pstm,
	ULARGE_INTEGER cb,ULARGE_INTEGER* pcbRead,ULARGE_INTEGER* pcbWritten){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::Commit(DWORD){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::Revert(void){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::Clone(IStream ** ppvstream){return E_NOTIMPL;}
HRESULT STDMETHODCALLTYPE CResourceStream::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
	ULARGE_INTEGER* lpNewFilePointer)
{
	LONGLONG newpostoset=readpos;
	switch(dwOrigin)
	{
	case STREAM_SEEK_SET:
		{
			newpostoset=liDistanceToMove.QuadPart;
		}
		break;
	case STREAM_SEEK_CUR:
		{
			newpostoset+=liDistanceToMove.QuadPart;
			newpostoset=max(0,newpostoset);
			newpostoset=min(newpostoset,size);
		}
		break;
	case STREAM_SEEK_END:
		{
			newpostoset=size-liDistanceToMove.QuadPart;
			newpostoset=max(0,newpostoset);
		}
		break;
	default:   
		return STG_E_INVALIDFUNCTION;
		break;
	}
	readpos=(DWORD)newpostoset;
	if(lpNewFilePointer) lpNewFilePointer->QuadPart=readpos;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CResourceStream::Stat(STATSTG* pStatstg, DWORD grfStatFlag)
{
	pStatstg->cbSize.QuadPart=size;
	return S_OK;
}
CComPtr<IStream> CResourceStream::CreateInstanse(HMODULE hModule,LPCWSTR lpName,LPCWSTR lpType)
{
	CComObjectNoLock<CResourceStream>* newone=new CComObjectNoLock<CResourceStream>();
	if(newone->GetRes(hModule,lpName,lpType))
	{
		return newone;
	}
	else
	{
		delete newone;
		return nullptr;
	}
}

void debugstring(LPTSTR pszFormat, ...)
{
	TCHAR szBuffer[1024];
	va_list pArgList;

	va_start(pArgList, pszFormat);
	_vstprintf_s(szBuffer, pszFormat, pArgList);
	va_end(pArgList);

	OutputDebugString(szBuffer);
}
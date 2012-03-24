#define Lib_Builder_Process
#include "cexarray.h"
#include <aclapi.h>
#include <DbgHelp.h>
#include <atlfile.h>
#include <netfw.h>

FileStream::FileStream(HANDLE hFile) 
{
	_refcount = 1;
	_hFile = hFile;
}

FileStream::~FileStream()
{
	if (_hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(_hFile);
	}
}

HRESULT FileStream::OpenFile(LPCWSTR pName, IStream ** ppStream,bool write)
{
	return CreateFile(pName, (write?GENERIC_WRITE:0x0)|GENERIC_READ, FILE_SHARE_READ,
		nullptr, write?OPEN_ALWAYS:OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,nullptr,ppStream);
}
HRESULT FileStream::CreateNewFile(LPCWSTR pName, IStream ** ppStream)
{
	return CreateFile(pName, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ,
		nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,nullptr,ppStream);
}
HRESULT FileStream::CreateFile( LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile,
	IStream ** ppStream)
{
	HANDLE hFile = ::CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (hFile == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	*ppStream = new FileStream(hFile);
	((FileStream*)*ppStream)->filename=lpFileName;
	if(*ppStream == nullptr)
		CloseHandle(hFile);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileStream::QueryInterface(REFIID iid, void ** ppvObject)
{ 
	if (iid == __uuidof(IUnknown)
		|| iid == __uuidof(IStream)
		|| iid == __uuidof(ISequentialStream))
	{
		*ppvObject = static_cast<IStream*>(this);
		AddRef();
		return S_OK;
	} else
		return E_NOINTERFACE; 
}

ULONG STDMETHODCALLTYPE FileStream::AddRef(void) 
{ 
	return (ULONG)InterlockedIncrement(&_refcount); 
}

ULONG STDMETHODCALLTYPE FileStream::Release(void) 
{
	ULONG res = (ULONG) InterlockedDecrement(&_refcount);
	if (res == 0) 
		delete this;
	return res;
}
HRESULT STDMETHODCALLTYPE FileStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	ULONG readed=0;
	BOOL rc = ReadFile(_hFile, pv, cb, &readed, nullptr);
	if(pcbRead!=nullptr)
		*pcbRead=readed;
	return (rc) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT STDMETHODCALLTYPE FileStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
	ULONG writed=0;
	BOOL rc = WriteFile(_hFile, pv, cb,&writed , nullptr);
	if(pcbWritten!=nullptr)
		*pcbWritten=writed;
	return rc ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT STDMETHODCALLTYPE FileStream::SetSize(ULARGE_INTEGER size)
{ 
	if (SetFilePointerEx(_hFile,*(LARGE_INTEGER*)&size,nullptr,
		FILE_BEGIN) == 0)
		return HRESULT_FROM_WIN32(GetLastError());
	if(::SetEndOfFile(_hFile))
		return S_OK; 
	else
		return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT STDMETHODCALLTYPE FileStream::CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead,
	ULARGE_INTEGER* pcbWritten) 
{ 
	if(pstm==nullptr) return STG_E_INVALIDPOINTER;
	ULONGLONG read=0;
	ULONGLONG written=0;
	char buf[1024];
	ULONG temp;
	while(read<cb.QuadPart)
	{
		if(this->Read(buf,sizeof(buf),&temp)==S_OK)
		{
			if(temp==0) break;
			read+=temp;
			ULONG temp2;
			HRESULT res=pstm->Write(buf,temp,&temp2);
			if(res==S_OK)
			{
				written+=temp2;
			}
			else
				return res;
		}
	}
	if(pcbRead) pcbRead->QuadPart=read;
	if(pcbWritten) pcbWritten->QuadPart=written;
	return S_OK;   
}

HRESULT STDMETHODCALLTYPE FileStream::Commit(DWORD grfCommitFlags)                                      
{
	if(grfCommitFlags==STGC_DEFAULT)
		return ::FlushFileBuffers(_hFile)==FALSE?E_FAIL:S_OK;
	else
		return E_FAIL;
}

HRESULT STDMETHODCALLTYPE FileStream::Revert(void)                                       
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE FileStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)              
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE FileStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)            
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE FileStream::Clone(IStream **)                                  
{ 
	return E_NOTIMPL;   
}

HRESULT STDMETHODCALLTYPE FileStream::Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
	ULARGE_INTEGER* lpNewFilePointer)
{ 
	DWORD dwMoveMethod;

	switch(dwOrigin)
	{
	case STREAM_SEEK_SET:
		dwMoveMethod = FILE_BEGIN;
		break;
	case STREAM_SEEK_CUR:
		dwMoveMethod = FILE_CURRENT;
		break;
	case STREAM_SEEK_END:
		dwMoveMethod = FILE_END;
		break;
	default:   
		return STG_E_INVALIDFUNCTION;
		break;
	}

	if (SetFilePointerEx(_hFile, liDistanceToMove, (PLARGE_INTEGER) lpNewFilePointer,
		dwMoveMethod) == 0)
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileStream::Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
{
	if(grfStatFlag!=STATFLAG_NONAME)
	{
		pStatstg->pwcsName=(LPOLESTR)CoTaskMemAlloc( (filename.GetLength()+1)*sizeof(WCHAR));
		wcscpy(pStatstg->pwcsName,filename);
	}
	BY_HANDLE_FILE_INFORMATION finfo;
	if(FALSE==GetFileInformationByHandle(_hFile,&finfo))
		return GetLastError();
	pStatstg->atime=finfo.ftLastAccessTime;
	pStatstg->ctime=finfo.ftCreationTime;
	pStatstg->mtime=finfo.ftLastWriteTime;
	pStatstg->clsid=CLSID_NULL;
	pStatstg->grfLocksSupported=0;
	pStatstg->grfMode=-1;
	pStatstg->type=STGTY_STREAM;
	pStatstg->cbSize.HighPart=finfo.nFileSizeHigh;
	pStatstg->cbSize.LowPart=finfo.nFileSizeLow;

	return S_OK;
}

void DebugTools::UseMiniDmpFile()
{
	::SetUnhandledExceptionFilter(SelfUnhandledException);
}
void DebugTools::UsePurecallHandler()
{
	_set_purecall_handler(SelfPurecallHandler);
}
void DebugTools::UseUnhandledExceptionFilter()
{
	::SetUnhandledExceptionFilter(SelfUnhandledExceptionFilter);
}
void DebugTools::SelfPurecallHandler(void)
{
	wchar_t filename[MAX_PATH];
	::GetModuleFileNameW(nullptr,filename,MAX_PATH);
	wcscat_s(filename,L".dmp");
	::MessageBox(nullptr,filename,L"PurecallHandler",0);
	CAtlFile file;
	file.Create(filename,FILE_WRITE_DATA,FILE_SHARE_READ,CREATE_ALWAYS);
	BOOL res=::MiniDumpWriteDump(::GetCurrentProcess(),::GetCurrentProcessId(),file,MiniDumpNormal,nullptr,nullptr,nullptr);
	file.Close();
	exit(0);
}
LONG WINAPI DebugTools::SelfUnhandledException(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	wchar_t path[MAX_PATH];
	::GetModuleFileNameW(nullptr,path,MAX_PATH);
	wcscat_s(path,L".dmp");
	HANDLE hDumpFile=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = GetCurrentThreadId();
	eInfo.ExceptionPointers = ExceptionInfo;
	eInfo.ClientPointers = FALSE;

	MINIDUMP_CALLBACK_INFORMATION cbMiniDump;
	cbMiniDump.CallbackRoutine = 0;
	cbMiniDump.CallbackParam = 0;

	MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hDumpFile,
		MiniDumpNormal,
		&eInfo,
		nullptr,
		&cbMiniDump);
	CloseHandle(hDumpFile);

	return EXCEPTION_EXECUTE_HANDLER;
}
LONG WINAPI DebugTools::SelfUnhandledExceptionFilter( __in  struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	DWORD temp;
	DWORD dwExceptCode = ExceptionInfo -> ExceptionRecord ->
		ExceptionCode;

	DWORD nThreadId = GetCurrentThreadId(); 
	DWORD nProcessId = GetCurrentProcessId();

	_EXCEPTION_POINTERS *p = ExceptionInfo;
	char errorinfo[256];
	sprintf_s( errorinfo,"Unknown WIN32 exception:%d at address 0x%.8x <ThreadId:%d, ProcessId:%d>",
		p->ExceptionRecord->ExceptionCode,
		p->ExceptionRecord->ExceptionAddress,
		nThreadId,
		nProcessId );
	wchar_t path[MAX_PATH];
	::GetModuleFileNameW(nullptr,path,MAX_PATH);
	wcscat_s(path,L".exception.txt");
	HANDLE hDumpFile=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
	WriteFile(hDumpFile,errorinfo,strlen(errorinfo),&temp,nullptr);
	CloseHandle(hDumpFile);

	return EXCEPTION_EXECUTE_HANDLER;
}

CShareMemory::CShareMemory(LPCWSTR name,DWORD size,BOOL shareWrite):memfile(INVALID_HANDLE_VALUE)
{
	filesize=size;

	DWORD dwRes;
	PSID pEveryoneSID = nullptr,pOwnerSID=nullptr;
	PACL pACL = nullptr;
	SECURITY_DESCRIPTOR SD;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
		SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthOwner =
		SECURITY_CREATOR_SID_AUTHORITY;
	SECURITY_ATTRIBUTES sa;
	EXPLICIT_ACCESS *acclist=nullptr;

	__try
	{
		if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
			SECURITY_WORLD_RID,
			0, 0, 0, 0, 0, 0, 0,
			&pEveryoneSID))
		{
			ATLTRACE("AllocateAndInitializeSid Error %u\n", GetLastError());
			__leave;
		}

		int eacount=0;
		if(shareWrite)
		{
			eacount=1;
			acclist=(EXPLICIT_ACCESS *)calloc(eacount,sizeof(EXPLICIT_ACCESS));
			if(acclist==nullptr) __leave;

			acclist[0].grfAccessPermissions = GENERIC_WRITE|GENERIC_READ;
			acclist[0].grfAccessMode = SET_ACCESS;
			acclist[0].grfInheritance= NO_INHERITANCE;
			acclist[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			acclist[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			acclist[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;
		}
		else
		{
			eacount=2;
			acclist=(EXPLICIT_ACCESS *)calloc(eacount,sizeof(EXPLICIT_ACCESS));
			if(acclist==nullptr) __leave;

			if(!AllocateAndInitializeSid(&SIDAuthOwner, 1,
				SECURITY_CREATOR_OWNER_RID,
				0, 0, 0, 0, 0, 0, 0,
				&pOwnerSID))
			{
				ATLTRACE("AllocateAndInitializeSid Error %u\n", GetLastError());
				__leave;
			}

			acclist[0].grfAccessPermissions = GENERIC_READ;
			acclist[0].grfAccessMode = SET_ACCESS;
			acclist[0].grfInheritance= NO_INHERITANCE;
			acclist[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			acclist[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			acclist[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

			acclist[1].grfAccessPermissions = GENERIC_READ|GENERIC_WRITE;
			acclist[1].grfAccessMode = SET_ACCESS;
			acclist[1].grfInheritance= NO_INHERITANCE;
			acclist[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
			acclist[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			acclist[1].Trustee.ptstrName  = (LPTSTR) pOwnerSID;
		}

		dwRes = SetEntriesInAcl(eacount, acclist, nullptr, &pACL);
		if (ERROR_SUCCESS != dwRes) 
		{
			ATLTRACE("SetEntriesInAcl Error %u\n", GetLastError());
			__leave;
		}

		// Initialize a security descriptor.

		if (!InitializeSecurityDescriptor(&SD,
			SECURITY_DESCRIPTOR_REVISION)) 
		{  
			ATLTRACE("InitializeSecurityDescriptor Error %u\n",
				GetLastError());
			__leave;
		} 

		// Add the ACL to the security descriptor. 
		if (!SetSecurityDescriptorDacl(&SD, 
			TRUE,     // bDaclPresent flag   
			pACL, 
			FALSE))   // not a default DACL 
		{  
			ATLTRACE("SetSecurityDescriptorDacl Error %u\n",
				GetLastError());
			__leave;
		} 

		// Initialize a security attributes structure.
		sa.nLength = sizeof (SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = &SD;
		sa.bInheritHandle = FALSE;

		memfile=CreateFileMappingW(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,0,filesize,name);
		if(memfile==INVALID_HANDLE_VALUE) __leave;
		if(GetLastError()==ERROR_ALREADY_EXISTS)
			ATLTRACE("file map exit\n");
		p=MapViewOfFileEx(memfile,FILE_MAP_READ|FILE_MAP_WRITE,0,0,filesize,nullptr);
	}
	__finally
	{
		if (pEveryoneSID) 
			FreeSid(pEveryoneSID);
		if (pOwnerSID)
			FreeSid(pOwnerSID);
		if (pACL) 
			LocalFree(pACL);
		if(acclist)
			free(acclist);
	}
}
CShareMemory::~CShareMemory(void)
{
	if(p!=nullptr) UnmapViewOfFile(p);
	if(INVALID_HANDLE_VALUE!=memfile) CloseHandle(memfile);
}
void* CShareMemory::GetBuffer()
{
	return p;
}

BOOL ToolCollection::EnableDebugPrivilege(BOOL fEnable)
{
	// Enabling the debug privilege allows the application to see
	// information about service applications
	BOOL fOk = FALSE;    // Assume function fails
	HANDLE hToken;

	// Try to open this process's access token
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, 
		&hToken)) {

			// Attempt to modify the "Debug" privilege
			TOKEN_PRIVILEGES tp;
			tp.PrivilegeCount = 1;
			LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
			tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
			fOk = (GetLastError() == ERROR_SUCCESS);
			CloseHandle(hToken);
	}
	return(fOk);
}
bool ToolCollection::RegisterURLProtocal(const CAtlStringW protocalname,const CAtlStringW cmdline)
{
	CAtlStringW temp,cmd,icon;
	::GetModuleFileNameW(nullptr,temp.GetBuffer(MAX_PATH),MAX_PATH);
	temp.ReleaseBuffer();
	cmd.Format(L"\"%s\" %s",temp,cmdline);
	icon.Format(L"\"%s\",1",temp);

	CRegKey regroot;
	bool rebuild=true;
	if(ERROR_SUCCESS==regroot.Open(HKEY_CLASSES_ROOT,protocalname))
	{
		CRegKey checkcmd;
		if(ERROR_SUCCESS==checkcmd.Open(regroot,L"shell\\open\\command"))
		{
			ULONG count=MAX_PATH+20;
			if(ERROR_SUCCESS==checkcmd.QueryStringValue(nullptr,temp.GetBuffer(count),&count))
			{
				temp.ReleaseBuffer();
				if(temp.CompareNoCase(cmd)==0)
				{
					rebuild=false;
				}
			}
			else
				temp.ReleaseBuffer();
		}
		if(rebuild==false)
		{
			DWORD index=0;
			ULONG count=260;
			rebuild=true;
			while(ERROR_SUCCESS==regroot.EnumKey(index,temp.GetBuffer(count),&count))
			{
				temp.ReleaseBuffer();
				index++;
				if(temp.CompareNoCase(L"URL Protocol")==0)
				{
					rebuild=false;
					break;
				}
			}
		}
	}
	if(rebuild)
	{
		temp.Format(L"URL:%s Protocol",protocalname);
		regroot.Close();
		if(regroot.Create(HKEY_CLASSES_ROOT,protocalname)==ERROR_SUCCESS)
		{
			regroot.SetStringValue(nullptr,temp);
			regroot.SetStringValue(L"URL Protocol",L"");
			CRegKey subkey;
			if(ERROR_SUCCESS==subkey.Create(regroot,L"DefaultIcon"))
			{
				subkey.SetStringValue(nullptr,icon);
				subkey.Close();
			}
			if(ERROR_SUCCESS==subkey.Create(regroot,L"shell\\open\\command"))
			{
				subkey.SetStringValue(nullptr,cmd);
				subkey.Close();
			}
		}
	}
	return true;
}

bool ToolCollection::RegisterAutoStart(const CAtlStringW keyname,const CAtlStringW cmdline)
{
	CAtlStringW temp,cmd;
	::GetModuleFileNameW(nullptr,temp.GetBuffer(MAX_PATH),MAX_PATH);
	temp.ReleaseBuffer();
	cmd.Format(L"\"%s\" %s",temp,cmdline);

	CRegKey key;
	if(ERROR_SUCCESS==key.Open(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"))
	{
		key.SetStringValue(keyname,cmd);
		key.Close();
		return true;
	}
	return false;
}
bool ToolCollection::RemoveAutoStart(const CAtlStringW keyname)
{
	CRegKey key;
	if(ERROR_SUCCESS==key.Open(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"))
	{
		key.DeleteValue(keyname);
		key.Close();
		return true;
	}
	return false;
}
void ToolCollection::AddSelfToWindowsFirewall(const CAtlStringW name)
{
	CComPtr<INetFwMgr> FwMgr;
	CComPtr<INetFwPolicy> FwPolocy;
	CComPtr<INetFwProfile> FwProfile;

	if(FAILED(FwMgr.CoCreateInstance( __uuidof(NetFwMgr),nullptr,CLSCTX_INPROC_SERVER))) return;
	if(FAILED(FwMgr->get_LocalPolicy(&FwPolocy))) return;
	if(FAILED(FwPolocy->get_CurrentProfile(&FwProfile))) return;
	VARIANT_BOOL res;
	if(FAILED(FwProfile->get_FirewallEnabled(&res))) return;
	if(res==VARIANT_FALSE) return;

	if(FAILED(FwProfile->get_ExceptionsNotAllowed(&res))) return;
	if(res==VARIANT_TRUE) return;

	CComPtr<INetFwAuthorizedApplication> FwApp;
	CComPtr<INetFwAuthorizedApplications> FwApps;
	if(FAILED(FwProfile->get_AuthorizedApplications(&FwApps))) return;

	CAtlStringW filename,fullfilename;
	::GetModuleFileNameW(nullptr,filename.GetBuffer(MAX_PATH),MAX_PATH);
	filename.ReleaseBuffer();
	DWORD size=::GetFullPathName(filename,0,0,0);
	size=GetFullPathName(filename,size,fullfilename.GetBuffer(size),nullptr);
	fullfilename.ReleaseBuffer();

	if(SUCCEEDED(FwApps->Item(CComBSTR(fullfilename),&FwApp)))
	{
		FwApp->put_Enabled(VARIANT_TRUE);
	}
	else
	{
		if(FAILED(FwApp.CoCreateInstance(__uuidof(NetFwAuthorizedApplication),
			nullptr,CLSCTX_INPROC_SERVER))) return;

		if(FAILED(FwApp->put_Name(CComBSTR(name)))) return;

		if(FAILED(FwApp->put_ProcessImageFileName(CComBSTR(fullfilename)))) return;
		if(FAILED(FwApp->put_Enabled(VARIANT_TRUE)) ) return;

		FwApps->Add(FwApp);
	}
}
#include "MapedFile.h"

MapedFile::MapedFile():m_fHanle(INVALID_HANDLE_VALUE),m_mapHandle(nullptr),AllocationGranularity(0),readonly(true)
{
	filesize.QuadPart=0;
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	AllocationGranularity=info.dwAllocationGranularity;
}
MapedFile::~MapedFile()
{
	if(m_mapHandle!=nullptr) CloseHandle(m_mapHandle);
	if(m_fHanle!=INVALID_HANDLE_VALUE) CloseHandle(m_fHanle);
}
LPMAPEDVIEW MapedFile::CreateViewObject(){return new MapedView(this);}

bool MapedFile::OpenFile(LPCWSTR filename,
						 BOOL create_if_not_exist/*=false*/,
						 BOOL readonly/*=TRUE*/,
						 DWORD mapsizelow/*=0*/,
						 DWORD mapsizehigh/*=0*/,
						 LPCWSTR mapname/*=nullptr*/)
{
	this->readonly=readonly;
	if(m_fHanle!=INVALID_HANDLE_VALUE || m_mapHandle!=nullptr) return false;
	m_fHanle=CreateFileW(filename,readonly?FILE_READ_DATA:FILE_READ_DATA|FILE_WRITE_DATA,
		FILE_SHARE_READ,0,create_if_not_exist?OPEN_ALWAYS:OPEN_EXISTING,0,0);
	if(m_fHanle==INVALID_HANDLE_VALUE)
	{
		lasterror=GetLastError();
		return false;
	}
	m_mapHandle=::CreateFileMappingW(m_fHanle,0,readonly?PAGE_READONLY:PAGE_READWRITE,mapsizehigh,
		mapsizelow,
		mapname);
	if(m_mapHandle==nullptr)
	{
		CloseHandle(m_fHanle);
		m_fHanle=INVALID_HANDLE_VALUE;
		lasterror=GetLastError();
		return false;
	}
	filesize.LowPart=::GetFileSize(m_fHanle,&filesize.HighPart);
	if(mapsizehigh!=0 && mapsizelow!=0)
	{
		ULARGE_INTEGER tempui;
		tempui.LowPart=mapsizelow;
		tempui.HighPart=mapsizehigh;
		filesize.QuadPart=min(filesize.QuadPart,tempui.QuadPart);
	}
	return true;
}
MapedView::MapedView(LPMAPEDFILE file):m_pFile(file)
{
	mapstart=nullptr;
	startpos.QuadPart=0;
	mapsize=0;
}
MapedView::~MapedView()
{
	Unmaped();
}
void MapedView::Unmaped()
{
	if(mapstart)
	{
		ATLVERIFY(UnmapViewOfFile(mapstart));
		mapstart=nullptr;
		startpos.QuadPart=0;
		mapsize=0;
	}
}
LPVOID MapedView::DoMap(ULONGLONG pos,DWORD size)
{
	char* ret=nullptr;

	ULARGE_INTEGER new_startpos;
	DWORD new_mapsize;

	//new_startpos.QuadPart=0;
	//new_mapsize=0;
	DWORD ag=m_pFile->GetAllocationGranularity();
	new_startpos.QuadPart=pos/ag*ag;
	if(m_pFile->GetFileSize()<new_startpos.QuadPart)
	{
		Unmaped();
		return nullptr;
	}
	ULONGLONG endpos=(pos+size+ag-1)/ag*ag;
	new_mapsize=(DWORD)(min(m_pFile->GetFileSize(),endpos)-new_startpos.QuadPart);

	if(new_startpos.QuadPart!=startpos.QuadPart || new_mapsize!=mapsize)
	{
		Unmaped();
		mapstart=MapViewOfFile(m_pFile->GetMapHandle(),
			m_pFile->IsReadOnly()?FILE_MAP_READ:FILE_MAP_WRITE,
			new_startpos.HighPart,new_startpos.LowPart,
			new_mapsize);
		if(mapstart==nullptr)
			return nullptr;

		startpos.QuadPart=new_startpos.QuadPart;
		mapsize=new_mapsize;
	}
	ret=((char*)mapstart)+(pos-startpos.QuadPart);
	return ret;
}
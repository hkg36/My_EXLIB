#pragma once
#include "cexarray.h"

class MapedFile;
class MapedView;
typedef CIPtr<MapedFile> LPMAPEDFILE;
typedef CIPtr<MapedView> LPMAPEDVIEW;
class MapedFile:public IPtrBase<MapedFile>
{
	friend class MapedView;
private:
	HANDLE m_fHanle;
	HANDLE m_mapHandle;
	ULARGE_INTEGER filesize;
	DWORD AllocationGranularity;

	HANDLE GetMapHandle(){return m_mapHandle;}
	ULONGLONG GetFileSize(){return filesize.QuadPart;}
	DWORD GetAllocationGranularity(){return AllocationGranularity;}

	MapedFile();
	BOOL readonly;
public:
	BOOL IsReadOnly(){return readonly;}
	DWORD lasterror;
	static LPMAPEDFILE CreateObject(){return new MapedFile();}
	LPMAPEDVIEW CreateViewObject();
	~MapedFile();

	bool OpenFile(LPCWSTR filename,
		BOOL create_if_not_exist=false,
		BOOL readonly=TRUE,
		DWORD mapsizelow=0,
		DWORD mapsizehigh=0,
		LPCWSTR mapname=nullptr);
};
class MapedView:public IPtrBase<MapedView,long>
{
	friend class MapedFile;
private:
	LPMAPEDFILE m_pFile;
	void* mapstart;
	ULARGE_INTEGER startpos;
	DWORD mapsize;
	MapedView(LPMAPEDFILE file);

public:
	~MapedView();
	DWORD MapedSize(){return mapsize;}
	LPVOID StartPoint(){return mapstart;}
	void Unmaped();
	LPVOID DoMap(ULONGLONG pos,DWORD size);
};
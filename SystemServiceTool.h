#pragma once
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
class CSystemService
{
private:
	typedef BOOL (_stdcall *ServiceFunction)();
	typedef BOOL (_stdcall *ServiceMessage)(MSG &msg);
	static ServiceFunction ServiceStart;
	static ServiceFunction ServiceIdle;
	static ServiceFunction ServiceStop;
	static ServiceMessage ServiceMSG;

public:
	static void SetServiceFunction(ServiceFunction start,
		ServiceFunction stop,
		ServiceFunction idle=nullptr,
		ServiceMessage message=nullptr);
	static void SetCanDirectRun(bool v)
	{
		can_direct_run=v;
	}
private:
	static SERVICE_STATUS_HANDLE hServiceStatus;     
	//DWORD ServiceCurrentStatus;
	static DWORD mainThreadid;
	static SERVICE_TABLE_ENTRY ServiceTable[2];
	static unsigned WINAPI DebugHelpProc(LPVOID lpParameter);
	static bool can_direct_run;
public:
	static int StartServiceMain(LPCWSTR servicename,int argc, TCHAR *argv[]);
	static LPCWSTR dependens;
private:
	static void WINAPI ServiceMain(DWORD argc,LPTSTR *argv);
	static DWORD WINAPI ServiceControl(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext);
	static BOOL   UpdateServiceStatus(DWORD   dwCurrentState,   DWORD   dwWin32ExitCode,   
		DWORD   dwServiceSpecificExitCode,   DWORD   dwCheckPoint,DWORD   dwWaitHint);
	static void Install(LPCWSTR displayname);
	static void Uninstall();
	static BOOL StopService(SC_HANDLE newService);
};

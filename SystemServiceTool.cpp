#include "SystemServiceTool.h"

CSystemService::ServiceFunction CSystemService::ServiceStart=nullptr;
CSystemService::ServiceFunction CSystemService::ServiceIdle=nullptr;
CSystemService::ServiceFunction CSystemService::ServiceStop=nullptr;
CSystemService::ServiceMessage CSystemService::ServiceMSG=nullptr;
SERVICE_STATUS_HANDLE CSystemService::hServiceStatus=nullptr;
DWORD CSystemService::mainThreadid=0;
SERVICE_TABLE_ENTRY CSystemService::ServiceTable[2]=
{nullptr,(LPSERVICE_MAIN_FUNCTION)CSystemService::ServiceMain,nullptr,nullptr};
bool CSystemService::can_direct_run=false;

void CSystemService::SetServiceFunction(ServiceFunction start,
										ServiceFunction stop,
										ServiceFunction idle,
										ServiceMessage message)
{
	ServiceStart=start;
	ServiceStop=stop;
	ServiceIdle=idle;
	ServiceMSG=message;
}

unsigned WINAPI CSystemService::DebugHelpProc(LPVOID lpParameter)
{
	getchar();
	PostThreadMessage(*(DWORD*)lpParameter,WM_QUIT,0,0);
	return 0;
}

int CSystemService::StartServiceMain(LPCWSTR servicename,int argc, TCHAR *argv[]) 
{
	ServiceTable[0].lpServiceName=const_cast<LPWSTR>(servicename);
	if(argc>=2)
	{
		if(_tcscmp(_T("-install"),argv[1])==0 || _tcscmp(_T("-i"),argv[1])==0)
		{
			if(argc>=3)
				Install(argv[2]);
			else
				Install(nullptr);
		}
		else if(_tcscmp(_T("-uninstall"),argv[1])==0 || _tcscmp(_T("-u"),argv[1])==0)
		{
			Uninstall();
		}
		else if(_tcscmp(_T("-s"),argv[1])==0)
		{
			if(!StartServiceCtrlDispatcher(ServiceTable))   
			{   
				printf("RegisterServer   First");   
			} 
		}
	}
	else
	{
		if(can_direct_run==false)
		{
			Install(nullptr);
			return 0;
		}
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
		DWORD threadid=GetCurrentThreadId();
		CloseHandle((HANDLE)_beginthreadex(0,0,DebugHelpProc,&threadid,0,0));
		if(ServiceStart && !ServiceStart()) return 1;
		BOOL bRet;
		MSG msg;
		while(true)
		{ 
			bRet = PeekMessage( &msg, 0, 0, 0, PM_REMOVE );
			if(bRet==FALSE)
			{
				if(ServiceIdle) ServiceIdle();
				bRet = GetMessage( &msg, 0, 0, 0 );
				if(bRet==0)break;
			}
			else
			{
				if(msg.message==WM_QUIT)
					break;
			}
			if (bRet == -1)
			{
				return 0;
			}
			else
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg);
				if(ServiceMSG)ServiceMSG(msg);
			}
		}
		if(ServiceStop && !ServiceStop()) return 2;
	}
	return 0;
}
void WINAPI CSystemService::ServiceMain(DWORD argc,LPTSTR *argv)   
{   
	mainThreadid=::GetCurrentThreadId();
	hServiceStatus = RegisterServiceCtrlHandlerEx(ServiceTable[0].lpServiceName,ServiceControl,nullptr);   
	/*if(!hServiceStatus ||!UpdateServiceStatus(SERVICE_START_PENDING,NO_ERROR,0,1,5*60*1000))   
	{   
		return;   
	}   
	if(ServiceStart && !ServiceStart())
	{
		UpdateServiceStatus(SERVICE_STOPPED,NO_ERROR,0,0,0);
		return;
	}*/
	if(!UpdateServiceStatus(SERVICE_RUNNING,NO_ERROR,0,0,0))   
	{   
		return;
	}
	if(ServiceStart && !ServiceStart())
	{
		UpdateServiceStatus(SERVICE_STOPPED,1,0,0,0);
		return;
	}
	BOOL bRet;
	MSG msg;
	while(true)
	{ 
		bRet = PeekMessage( &msg, 0, 0, 0, PM_REMOVE );
		if(bRet==FALSE)
		{
			if(ServiceIdle) ServiceIdle();
			bRet = GetMessage( &msg, 0, 0, 0 );
			if(bRet==0)break;
		}
		else
		{
			if(msg.message==WM_QUIT)
				break;
		}
		if (bRet == -1)
		{
			return;
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
			if(ServiceMSG) ServiceMSG(msg);
		}
	}
	UpdateServiceStatus(SERVICE_STOP_PENDING,NO_ERROR,0,1,3000);
	if(ServiceStop && !ServiceStop()) return;
	UpdateServiceStatus(SERVICE_STOPPED,NO_ERROR,0,0,0);
}   

DWORD WINAPI CSystemService::ServiceControl(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext)
{   
	switch(dwControl)   
	{   
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		PostThreadMessage(mainThreadid,WM_QUIT,0,0);break; 
	default:   
		break;   
	}
	return NO_ERROR;
}

BOOL CSystemService::UpdateServiceStatus(DWORD   dwCurrentState,   DWORD   dwWin32ExitCode,   
										 DWORD   dwServiceSpecificExitCode,   DWORD   dwCheckPoint,DWORD   dwWaitHint)   
{   
	SERVICE_STATUS   ServiceStatus;   
	ServiceStatus.dwServiceType   =   SERVICE_WIN32_OWN_PROCESS;   
	ServiceStatus.dwCurrentState   =   dwCurrentState;   

	if(dwCurrentState   ==   SERVICE_START_PENDING)  
	{   
		ServiceStatus.dwControlsAccepted   =   0;   
	}   
	else   
	{   
		ServiceStatus.dwControlsAccepted   =  SERVICE_ACCEPT_STOP   |   SERVICE_ACCEPT_SHUTDOWN;   
	}   

	if(dwServiceSpecificExitCode   ==   0)   
	{   
		ServiceStatus.dwWin32ExitCode   =   dwWin32ExitCode;   
	}   
	else   
	{   
		ServiceStatus.dwWin32ExitCode   =   ERROR_SERVICE_SPECIFIC_ERROR;   
	}   

	ServiceStatus.dwServiceSpecificExitCode   =   dwServiceSpecificExitCode;   
	ServiceStatus.dwCheckPoint   =   dwCheckPoint;   
	ServiceStatus.dwWaitHint   =   dwWaitHint;   

	if(!SetServiceStatus(hServiceStatus,&ServiceStatus))   
	{   
		//PostThreadMessage(mainThreadid,WM_QUIT,0,0);   
		return   FALSE;   
	}   

	return   TRUE;   
}
LPCWSTR CSystemService::dependens=L"Tcpip\0Dnscache\0";
void CSystemService::Install(LPCWSTR displayname)
{
	WCHAR startcmd[MAX_PATH+5];
	TCHAR filename[MAX_PATH];
	if(0==::GetModuleFileName(nullptr,filename,MAX_PATH)) return;
	if(0==swprintf_s(startcmd,L"\"%s\" -s",filename))return;

	SC_HANDLE newService=nullptr, scm=nullptr;
	SERVICE_STATUS status;
	__try
	{
		scm = OpenSCManager(0, 0,
			SC_MANAGER_CREATE_SERVICE);
		if (!scm)
			__leave;
		// Install the new service
		newService = OpenServiceW(scm,ServiceTable[0].lpServiceName,SERVICE_ALL_ACCESS);
		if(newService)
		{
			QUERY_SERVICE_CONFIG* querybuff=nullptr;
			DWORD bufsz=0;
			if(QueryServiceConfig(newService,querybuff,0,&bufsz)==FALSE)
			{
				if(ERROR_INSUFFICIENT_BUFFER==GetLastError())
				{
					querybuff=(QUERY_SERVICE_CONFIG*)malloc(bufsz);
					if(querybuff)
					{
						if(QueryServiceConfig(newService,querybuff,bufsz,&bufsz))
						{
							wchar_t* firstQuotes=nullptr,*secondQuotes=nullptr;
							firstQuotes=wcschr(querybuff->lpBinaryPathName,'"');
							if(firstQuotes)
							{
								secondQuotes=wcschr(firstQuotes+1,'"');
								if(secondQuotes)
								{
									wchar_t tempfilename[MAX_PATH];
									wcsncpy_s(tempfilename,firstQuotes+1,secondQuotes-firstQuotes-1);
									if(_wcsicmp(tempfilename,filename)!=0)
									{
										if(StopService(newService))
										{
											if(CopyFile(filename,tempfilename,FALSE))
											{
												wcscpy_s(startcmd,querybuff->lpBinaryPathName);
											}
										}
									}
								}
							}
						}
						free(querybuff);
					}
				}
			}
			if(!ChangeServiceConfigW(newService,
				SERVICE_WIN32_SHARE_PROCESS|SERVICE_INTERACTIVE_PROCESS,
				SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL,
				startcmd,
				0,0,dependens,0,0,0))
				__leave;
		}
		else
		{
			newService = CreateServiceW(
				scm, ServiceTable[0].lpServiceName,
				displayname?displayname:ServiceTable[0].lpServiceName,
				SERVICE_ALL_ACCESS,
				SERVICE_WIN32_SHARE_PROCESS|SERVICE_INTERACTIVE_PROCESS,
				SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL,
				startcmd,
				0, 0, dependens, 0, 0);
		}
		if (newService)
		{
			SERVICE_FAILURE_ACTIONS sfa;
			ZeroMemory(&sfa,sizeof(sfa));
			sfa.dwResetPeriod=INFINITE;
			sfa.cActions=3;
			SC_ACTION sact[3];
			ZeroMemory(sact,sizeof(sact));
			sfa.lpsaActions=sact;
			sact[0].Delay=500;
			sact[0].Type=SC_ACTION_RESTART;
			sact[1].Delay=500;
			sact[1].Type=SC_ACTION_RESTART;
			sact[2].Delay=500;
			sact[2].Type=SC_ACTION_RESTART;
			ChangeServiceConfig2(newService,SERVICE_CONFIG_FAILURE_ACTIONS,&sfa);

			if(QueryServiceStatus(newService,&status))
			{
				if(status.dwCurrentState==SERVICE_STOPPED)
				{
					StartService(newService,0,0);
				}
			}
		}
	}
	__finally
	{
		if(newService)CloseServiceHandle(newService);
		if(scm)CloseServiceHandle(scm);
	}
}
void CSystemService::Uninstall()
{
	BOOL success;
	SC_HANDLE newService=nullptr, scm=nullptr;

	__try
	{
		scm = OpenSCManager(0, 0,
			SC_MANAGER_CREATE_SERVICE);
		if (!scm)
			__leave;

		newService = OpenService(
			scm, ServiceTable[0].lpServiceName,
			SERVICE_ALL_ACCESS | DELETE);
		if (!newService)
			__leave;

		if(StopService(newService)==FALSE) __leave;
		// Remove the service
		success = DeleteService(newService);
	}
	__finally
	{
		// Clean up
		if(newService)CloseServiceHandle(newService);
		if(scm)CloseServiceHandle(scm);
	}
}

BOOL CSystemService::StopService(SC_HANDLE newService)
{
	SERVICE_STATUS status;
	BOOL success=FALSE;
	__try
	{
		success = QueryServiceStatus(newService, &status);
		if (!success)
			__leave;
		if (status.dwCurrentState != SERVICE_STOPPED)
		{
			success = ControlService(newService,
				SERVICE_CONTROL_STOP, 
				&status);
			if (!success)
				return FALSE;
			int waitcount=0;
			while(QueryServiceStatus(newService,&status))
			{
				if(status.dwCurrentState==SERVICE_STOP_PENDING)
				{
					Sleep(1000);
					waitcount++;
					if(waitcount>10)
					{
						__leave;
					}
				}
				else
				{
					return TRUE;
				}
			}
		}
		else
			return TRUE;
	}
	__finally
	{
		
	}
	return FALSE;
}
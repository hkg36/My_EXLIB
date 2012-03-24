// MainFrm.h : interface of the CSocketFrame class
//
/////////////////////////////////////////////////////////////////////////////
#include <WinSock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>
#include <atlmisc.h>
#include <atlwin.h>
#include <atlcrack.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlcoll.h>
#include "cexarray.h"
#include "DataQueue.h"
#pragma once
const DWORD WM_SOCKET_EVENT=WM_USER+0xf001;
const DWORD SOCKET_CHECK=0xf001;
class CSocketFrame;
class CSyncSocket:public IPtrBase<CSyncSocket,long>
{
	friend CIPtr<CSyncSocket> CreateSocket(int af,
						  int type,
						  int protocol,
						  LPWSAPROTOCOL_INFO lpProtocolInfo,
						  GROUP g,
						  DWORD dwFlags,
						  CIPtr<CSyncSocket> emptysocket);
protected:
	SOCKET m_s;
	bool m_haserror;
	sockaddrstore m_addr;
	static int CALLBACK ConditionFunc(
		IN LPWSABUF lpCallerId,
		IN LPWSABUF lpCallerData,
		IN OUT LPQOS lpSQOS,
		IN OUT LPQOS lpGQOS,
		IN LPWSABUF lpCalleeId,
		OUT LPWSABUF lpCalleeData,
		OUT GROUP FAR *g,
		IN DWORD_PTR dwCallbackData
		)
	{
		CSyncSocket* p=(CSyncSocket*)dwCallbackData;
		return p->AcceptCondition(lpCallerId,lpCallerData,lpSQOS,lpGQOS,lpCalleeId,lpCalleeData,g);
	}
	void Init(const SOCKET s);
	CDataQueue m_buffer;
	void SendBufferData();
	bool busy;
public:
	bool IsBusy(){return busy;}
	void GetAddr(sockaddrstore &addr)
	{
		addr=m_addr;
	}
	void SetAddr(const sockaddr* address,int addlen){memcpy_s(&m_addr,sizeof(m_addr),address,addlen);}
	const sockaddrstore& GetAddr()const{return m_addr;}
	CStringA GetAddrStr(const sockaddr* addr=nullptr,int addrlen=0)const;
	SOCKET GetSocket(){return m_s;}
	bool IsError(){return m_haserror;}
	bool SetSockOpt(int level,int optname,const char* optval,int optlen)
	{
		return 0==setsockopt(m_s,level,optname,optval,optlen);
	}

	virtual void SetError()
	{
		m_haserror=true;
		Close();
	}
	CSyncSocket();
	CIPtr<CSyncSocket> Accept(CIPtr<CSyncSocket> emptysocket);
	virtual ~CSyncSocket();
	void Close();
	bool Bind(sockaddr* local,int locallen);
	bool Listen();
	bool Connect(const sockaddr* remote,int remotelen);
	bool Connect(LPCWSTR host,LPCWSTR port);
	//only wrap for send call
	//return true,go next send,return false,do not send any more this time
	//there maybe some error or need to wait OnWriteNext call if not overwrite OnWrite()
	bool WriteWrap(const char* start,int len);
	int Read(char *buffer,int bufsz);
	virtual void OnWriteBufferFinish(){}
	virtual void OnRead(){}
	virtual void OnWrite();
	virtual void OnOOB(){}
	virtual void OnAccept(){}
	virtual void OnConnect(){}
	virtual void OnClose();
	virtual void OnQOS(){}
	virtual void OnGroupQOS(){}
	virtual void OnRoutingInterfaceChange(){}
	virtual void OnAddressListChange(){}
	virtual void OnError(DWORD errorcode,DWORD eventcode)
	{
		CStringW temp;
		temp.Format(L"OnError errorcode(%u) eventcode(%u)",errorcode,eventcode);
		OutPutInfo(temp);
		SetError();
	}
	virtual void OnTimer(DWORD time){}
	virtual int AcceptCondition(IN LPWSABUF lpCallerId,
		IN LPWSABUF lpCallerData,
		IN OUT LPQOS lpSQOS,
		IN OUT LPQOS lpGQOS,
		IN LPWSABUF lpCalleeId,
		OUT LPWSABUF lpCalleeData,
		OUT GROUP FAR *g)
	{
		return CF_ACCEPT;
	}
	virtual CStringW GetInfo(DWORD infoid){return CStringW();}
	virtual void OutPutInfo(CStringW info){};
};
typedef CIPtr<CSyncSocket> LPSyncSocket;

CIPtr<CSyncSocket> CreateSocket(int af,
						  int type,
						  int protocol,
						  LPWSAPROTOCOL_INFO lpProtocolInfo,
						  GROUP g,
						  DWORD dwFlags,
						  CIPtr<CSyncSocket> emptysocket=new CSyncSocket());
class CSocketFrame : public CFrameWindowImpl<CSocketFrame>
{
private:
	friend class SocketFramAutoTool;
	static CSocketFrame *onlyone;
	ATL::CRBMap<SOCKET,LPSyncSocket> socketmap;
	CSocketFrame(){};
	~CSocketFrame(){};
public:
	DECLARE_FRAME_WND_CLASS(nullptr, 0)

	BEGIN_MSG_MAP_EX(CSocketFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SOCKET_EVENT, OnSocketEvent)
		MSG_WM_TIMER(OnTimer);
		CHAIN_MSG_MAP(CFrameWindowImpl<CSocketFrame>)
	END_MSG_MAP()

	bool AddSocket(LPSyncSocket newsocket);
	void RemoveSocket(SOCKET s);
	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	static CSocketFrame *GetFramWindow(){return onlyone;}
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	void OnTimer(UINT_PTR id);
	LRESULT OnSocketEvent(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
};
class SocketFramAutoTool
{
private:
	CSocketFrame frame;
public:
	SocketFramAutoTool(){frame.CreateEx(HWND_MESSAGE);}
	~SocketFramAutoTool()
	{
		if(frame.IsWindow())
			frame.DestroyWindow();
	}
};
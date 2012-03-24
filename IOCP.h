#pragma once
#include <WinSock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <Windows.h>
#include <Mswsock.h>
#include <atlcoll.h>
#include "extools.h"
namespace IOCP
{
	class IOCP_Item:public IPtrBase<IOCP_Item>
	{
		friend class IOCP_Shell;
	private:
		LPOVERLAPPED ov;
		BOOL ErrorState;
	public:
		LPOVERLAPPED GetOV(){return ov;}
		LPOVERLAPPED GetZeroOV()
		{
			ZeroMemory(ov,sizeof(OVERLAPPED));
			return ov;
		}
		template<class Type>
		bool BindIOCP(Type handle){return BindToIOCP((HANDLE)handle);}
		bool BindToIOCP(HANDLE handle);

		virtual void SetError()
		{
			ErrorState=TRUE;
		}
		BOOL IsError(){return ErrorState;}
		IOCP_Item();

		virtual ~IOCP_Item();
		virtual int IOComplete(DWORD trans_bytes)=0;
		virtual int IOFail()=0;
		virtual bool TimeCheck(DWORD /*now*/){return true;}
		virtual void ErrorMessage(DWORD /*id*/,CAtlStringW /*from*/,CAtlStringW /*message*/){} 
	};
	typedef CIPtr<IOCP_Item> LP_IOCP_Item;
	typedef CRBMap<LPOVERLAPPED,LP_IOCP_Item> ItemList;
	class IOCP_Shell
	{
		friend class IOCP_Item;
		friend class IOCP_Socket;
	private:
		static HANDLE iocp;
		static vector<HANDLE> threads;
		static bool runing;
		static unsigned WINAPI WorkThread(LPVOID timer);
		static void TimerCheck();

		static ItemList items;
		static ReadWriteLock itemslock;
		static LPFN_TRANSMITPACKETS fn_transmitpackets;
		static LPFN_DISCONNECTEX fn_disconnectex;
		static LPFN_CONNECTEX fn_connectex;
		static CDataBlockAllocor<OVERLAPPED> ovallocer;
	public:
		static size_t GetItemCount(){return items.GetCount();}
		static void Init(int threadcount=0);
		static void Dispose();
		static void RemoveItem(LPOVERLAPPED pov);
		static LP_IOCP_Item FindItem(LPOVERLAPPED pov);
		static LPOVERLAPPED AllocOv();
		static void FreeOv(LPOVERLAPPED pov);
	};

	class TcpSocketPool
	{
	private:
		static vector<SOCKET> socketlist;
		static CCriticalLock lock;
		static vector<SOCKET> socketlist6;
		static CCriticalLock lock6;
	public:
		static SOCKET GetSocket(int af);
		static void ReleaseSocket(SOCKET s,int af);
	};
	class IOCP_Socket:public IOCP_Item
	{
	protected:
		CAtlArray<char> buffer;
		SOCKET s;
		WSABUF wbuf;
		DWORD transed;
		DWORD flag;
		sockaddrstore addr;
		int addrlen;
		//int state;
		DWORD actionTick;
		//enum {NONE=0,RECV,SEND,SHUTDOWN,CONNECT};
		void SetActionTick();
		void CloseSocket();
		//TRANSMIT_PACKETS_ELEMENT tranPackele[2];

		DWORD timeoutSpan;
		DWORD closeTime;
		bool SocketClosed();
		void Init();
		typedef int (IOCP_Socket::*IOFun)(DWORD);
		IOFun nowiofun;
	public:
		CAtlStringW GetSockNameInfo();
		void SetNextIOFunction(IOFun funtion){nowiofun=funtion;}
		int SetSocketOpt( int level,int optname,const char* optval,int optlen);
		virtual void InitSocket(SOCKET s,const sockaddr *addr,int addrlen);
		virtual void InitSocket(ADDRESS_FAMILY af,int type,int protocol,bool out_connect=false);
		~IOCP_Socket();
		inline char* AllocBuffer(int size)
		{
			buffer.SetCount(size);
			wbuf.buf=buffer.GetData();
			wbuf.len=buffer.GetCount();
			return wbuf.buf;
		}
		inline char* GetBuffer(){return buffer.GetData();}
		inline size_t BufferSize(){return buffer.GetCount();}
		bool WriteSendPack(char *buf,int sz);
		bool StartConnect(const struct sockaddr* name,
			int namelen,PVOID lpSendBuffer,DWORD dwSendDataLength);
		void ShutDown();
		bool StartRecv(ULONG recvsize=-1);
		bool StartRecv(WSABUF* bufs,DWORD bufcount);
		bool StartSend(DWORD size);
		bool StartSend(WSABUF* bufs,DWORD bufcount);
		bool StartRecvSize(ULONG len);
		virtual void OnRecvSizeFinish(DWORD recved_bytes);
	public:
		virtual bool RunFirstAction();
		virtual int OnConnect(DWORD trans_bytes);
		virtual int OnRecv(DWORD trans_bytes);
		virtual int OnSend(DWORD trans_bytes);
		virtual int OnShutDown(DWORD trans_bytes);
		virtual int IOComplete(DWORD trans_bytes);
		virtual int IOFail();
		virtual bool TimeCheck(DWORD now);
	};

	typedef IOCP_Socket* (*CreateSocketFunc)();
	typedef bool (*CanAcceptNow)();
	class SocketSyncAccept:public IOCP_Item
	{
	private:
		SOCKET accepter;
		SOCKET accepted;
		CanAcceptNow canaccept;
		CreateSocketFunc csfunc;
		CCriticalLock lock;
		struct AcceptBuffer
		{
			UCHAR local[sizeof(sockaddrstore)*2+16*2];
		};
		AcceptBuffer acceptbuf;
		DWORD transed;
		static IOCP_Socket* defaultCreateSocket()
		{
			return new IOCP_Socket();
		}
		int af;
		CAtlStringW listenport;
	public:
		SocketSyncAccept(CreateSocketFunc csfunc=defaultCreateSocket,CanAcceptNow canaccept=nullptr);
		~SocketSyncAccept();
		bool Start(LPCWSTR port=nullptr,int af=AF_INET);
		void Stop();
		bool AcceptNext();
		int IOComplete(DWORD trans_bytes);
		int IOFail();
		bool TimeCheck(DWORD now);
	};
	typedef CIPtr<SocketSyncAccept> LPSOCKSyncAccept;
}
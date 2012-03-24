#include "IOCP.h"

namespace IOCP
{
	SOCKET CreateTcpSocketFromPool(int af);

	IOCP_Item::IOCP_Item()
	{
		ErrorState=FALSE;
		ov=IOCP_Shell::AllocOv();
	}
	IOCP_Item::~IOCP_Item()
	{
		IOCP_Shell::FreeOv(ov);
	}
	bool IOCP_Item::BindToIOCP(HANDLE handle)
	{
		CAutoWriteLock al(IOCP_Shell::itemslock);
		IOCP_Shell::items.SetAt(this->GetOV(),this);
		HANDLE resh=::CreateIoCompletionPort(handle,IOCP_Shell::iocp,(ULONG_PTR)this,0);
		if(resh==nullptr)
		{
			CSysErrorInfo info(GetLastError());
			ErrorMessage(GetLastError(),CStringW(info),__FUNCTION__);
			//87 不要认为是绑定错误，可能是重用的 socket
		}
		return true;
	}

	HANDLE IOCP_Shell::iocp=nullptr;
	vector<HANDLE> IOCP_Shell::threads;
	bool IOCP_Shell::runing=false;
	ItemList IOCP_Shell::items;
	ReadWriteLock IOCP_Shell::itemslock;
	LPFN_TRANSMITPACKETS IOCP_Shell::fn_transmitpackets=nullptr;
	LPFN_DISCONNECTEX IOCP_Shell::fn_disconnectex=nullptr;
	LPFN_CONNECTEX IOCP_Shell::fn_connectex=nullptr;
	CDataBlockAllocor<OVERLAPPED> IOCP_Shell::ovallocer;

	unsigned WINAPI IOCP_Shell::WorkThread(LPVOID timer)
	{
		DWORD CheckSpan=INFINITE;
		BOOL withtimercheck=(BOOL)timer;
		DWORD lastcheck=0;
		if(withtimercheck)
		{
			CheckSpan=10*1000;
		}
		while(true)
		{
			DWORD transed;
			//IOCP_Item* pitem=nullptr;
			ULONG_PTR key;
			LPOVERLAPPED pov=nullptr;
			BOOL res;
			res=::GetQueuedCompletionStatus(iocp,&transed,&key,&pov,CheckSpan);
			if(!runing) break;
			if(withtimercheck && res==FALSE)
			{
				if(WAIT_TIMEOUT==::GetLastError())
				{
					lastcheck=::GetTickCount();
					TimerCheck();
					continue;
				}
			}
			{
				LP_IOCP_Item pitem=FindItem(pov);
				//pitem=IOCP_Item::GetItemAddrFromOV(pov);
				if(pitem)
				{
					if(!pitem->IsError())
					{
						if(res==FALSE)
							pitem->IOFail();
						else
							pitem->IOComplete(transed);
						if(pitem->IsError())
						{
							RemoveItem(pov);
						}
					}
					else
					{
						RemoveItem(pov);
					}
				}
			}
			if(withtimercheck)
			{
				if(::GetTickCount()-lastcheck>CheckSpan)
				{
					lastcheck=::GetTickCount();
					TimerCheck();
				}
			}
		}
		return 0;
	}
	void IOCP_Shell::TimerCheck()
	{
		DWORD now=::GetTickCount();
		CAutoWriteTryLock al(itemslock);
		if(al.Locked()==false) return;
		auto pos=items.GetHeadPosition();
		while(pos!=nullptr)
		{
			auto oldpos=pos;
			auto *ipair=items.GetNext(pos);
			if(false==ipair->m_value->TimeCheck(now))
			{
				items.RemoveAt(oldpos);
			}
		}
	}
	void IOCP_Shell::Init(int threadcount)
	{
		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		runing=true;
		iocp=CreateIoCompletionPort(INVALID_HANDLE_VALUE,0,0,0);
		if(threadcount<=0)
		{
			threadcount=sysinfo.dwNumberOfProcessors*2;
		}
		if(threadcount<=0)
			threadcount=2;
		bool withtimer=true;
		unsigned id;
		HANDLE thread;
		thread=(HANDLE)_beginthreadex(0,0,WorkThread,(void*)withtimer,0,&id);
		if(thread)
			threads.push_back(thread);
		withtimer=false;
		for(int i=1;i<threadcount;i++)
		{
			thread=(HANDLE)_beginthreadex(0,0,WorkThread,(void*)withtimer,0,&id);
			if(thread)
				threads.push_back(thread);
		}

		SOCKET temps=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		GUID transpack_id=WSAID_TRANSMITPACKETS;
		DWORD transed;
		if(SOCKET_ERROR==WSAIoctl(temps,SIO_GET_EXTENSION_FUNCTION_POINTER,&transpack_id,sizeof(transpack_id),
			&fn_transmitpackets,sizeof(fn_transmitpackets),&transed,0,0))
			transed=::WSAGetLastError();
		GUID disconex_id=WSAID_DISCONNECTEX;
		if(SOCKET_ERROR==WSAIoctl(temps,SIO_GET_EXTENSION_FUNCTION_POINTER,&disconex_id,sizeof(disconex_id),
			&fn_disconnectex,sizeof(fn_disconnectex),&transed,0,0))
			transed=::WSAGetLastError();
		GUID connectex_id=WSAID_CONNECTEX;
		if(SOCKET_ERROR==WSAIoctl(temps,SIO_GET_EXTENSION_FUNCTION_POINTER,&connectex_id,sizeof(connectex_id),
			&fn_connectex,sizeof(fn_connectex),&transed,0,0))
			transed=::WSAGetLastError();
		closesocket(temps);
	}
	LP_IOCP_Item IOCP_Shell::FindItem(LPOVERLAPPED pov)
	{
		CAutoReadLock al(itemslock);
		auto i=items.Lookup(pov);
		if(i==nullptr)
			return nullptr;
		else
			return i->m_value;
	}
	void IOCP_Shell::Dispose()
	{
		{
			CAutoWriteLock al(itemslock);
			items.RemoveAll();
		}
		runing=false;

		for(size_t i=0;i<threads.size();i++)
		{
			::PostQueuedCompletionStatus(iocp,0,0,0);
		}
		::WaitForMultipleObjects(threads.size(),&threads[0],TRUE,INFINITE);
		for(size_t i=0;i<threads.size();i++)
		{
			CloseHandle(threads[i]);
		}
		CloseHandle(iocp);
	}
	void IOCP_Shell::RemoveItem(LPOVERLAPPED pov)
	{
		CAutoWriteLock al(itemslock);
		items.RemoveKey(pov);
	}
	LPOVERLAPPED IOCP_Shell::AllocOv()
	{
		return ovallocer.AllocBlock();
	}
	void IOCP_Shell::FreeOv(LPOVERLAPPED pov)
	{
		ovallocer.FreeBlock(pov);
	}

	CAtlStringW IOCP_Socket::GetSockNameInfo()
	{
		WCHAR hostname[NI_MAXHOST];
		WCHAR servInfo[NI_MAXSERV];
		if(0==GetNameInfoW(&addr,sizeof(addr),hostname,
			NI_MAXHOST,servInfo,NI_MAXSERV,NI_NUMERICHOST|NI_NUMERICSERV))
		{
			CAtlStringW temp;
			temp.Format(L"%s:%s",hostname,servInfo);
			return temp;
		}
		else
		{
			CSysErrorInfo info(WSAGetLastError());
			return (LPCWSTR)info;
		}
	}
	void IOCP_Socket::SetActionTick()
	{
		actionTick=::GetTickCount();
	}
	void IOCP_Socket::CloseSocket()
	{
		if(s!=INVALID_SOCKET)
		{
			closesocket(s);
			s=INVALID_SOCKET;
		}
	}
	bool IOCP_Socket::SocketClosed()
	{
		return s==INVALID_SOCKET;
	}
	int IOCP_Socket::SetSocketOpt( int level,int optname,const char* optval,int optlen)
	{
		return setsockopt(this->s,level,optname,optval,optlen);

	}
	void IOCP_Socket::InitSocket(SOCKET s,const sockaddr *addr,int addrlen)
	{
		this->addrlen=addrlen;
		memcpy(&this->addr,addr,addrlen);
		this->s=s;
		Init();
	}
	void IOCP_Socket::InitSocket(ADDRESS_FAMILY af,int type,int protocol,bool out_connect)
	{
		ATLASSERT(type==SOCK_STREAM && protocol==IPPROTO_TCP);
		//如果需要支持其他选项,必须实现其他socket pool.一般不需要支持其他协议,将来可能要支持ipv6
		this->s=CreateTcpSocketFromPool(af);
		if(s==SOCKET_ERROR)
		{
			SetError();
			return;
		}
		if(out_connect)
		{
			sockaddrstore bindfix;
			ZeroMemory(&bindfix,sizeof(bindfix));
			bindfix.sa_family=af;
			if(0==bind(s,(const sockaddr*)&bindfix,sizeof(bindfix)))
			{
				ATLTRACE(L"bind fail:%s\r\n",CSysErrorInfo(WSAGetLastError()));
			}
		}
		Init();
	}
	void IOCP_Socket::Init()
	{
		flag=0;
		timeoutSpan=3*60*1000;
		closeTime=0;
		actionTick=0;
		SetActionTick();
		nowiofun=nullptr;
		BindIOCP(s);
	}
	IOCP_Socket::~IOCP_Socket()
	{
		CloseSocket();
	}
	bool IOCP_Socket::RunFirstAction()
	{
		return StartRecv();
	}
	int IOCP_Socket::OnConnect(DWORD /*trans_bytes*/)
	{
		setsockopt( s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0 );
		if(RunFirstAction()==false)
		{
			SetError();
		}
		return 0;
	}
	bool IOCP_Socket::StartRecvSize(ULONG len)
	{
		AllocBuffer(len);
		StartRecv(&wbuf,1);
		return true;
	}
	void IOCP_Socket::OnRecvSizeFinish(DWORD /*recved_bytes*/)
	{
	}
	int IOCP_Socket::OnRecv(DWORD trans_bytes)
	{
		wbuf.len-=trans_bytes;
		wbuf.buf+=trans_bytes;
		if(wbuf.len!=0)
		{
			StartRecv(&wbuf,1);
		}
		else
		{
			DWORD trans_size=wbuf.buf-buffer.GetData();
			wbuf.buf=buffer.GetData();
			wbuf.len=buffer.GetCount();
			OnRecvSizeFinish(trans_size);
		}
		return 0;
	}
	int IOCP_Socket::OnSend(DWORD /*trans_bytes*/)
	{
		return 0;
	}
	int IOCP_Socket::OnShutDown(DWORD /*trans_bytes*/)
	{
		//CloseSocket();
		if(s!=INVALID_SOCKET)
		{
			TcpSocketPool::ReleaseSocket(s,addr.sa_family);
			s=INVALID_SOCKET;
		}
		SetError();
		return 0;
	}
	void IOCP_Socket::ShutDown()
	{
		SetActionTick();
		if(IOCP_Shell::fn_disconnectex)
		{
			//this->state=SHUTDOWN;
			SetNextIOFunction(&IOCP_Socket::OnShutDown);
			if(!IOCP_Shell::fn_disconnectex(s,this->GetZeroOV(),TF_REUSE_SOCKET,0))
			{
				if(ERROR_IO_PENDING!=WSAGetLastError())
				{
					SetError();
					CloseSocket();
				}
				else
				{
					timeoutSpan=480*1000;
				}
			}
		}
		else
		{
			SetError();
			CloseSocket();
		}
	}
	int IOCP_Socket::IOComplete(DWORD trans_bytes)
	{
		ATLASSERT(nowiofun);
#ifdef _DEBUG
		IOFun temp_pfun=nowiofun;
		nowiofun=nullptr;
		int res=(this->*temp_pfun)(trans_bytes);
		return res;
#else
		return (this->*nowiofun)(trans_bytes);
#endif
		/*
		int ret=0;
		switch(state)
		{
		case RECV:
		{
		ret=OnRecv(trans_bytes);
		}break;
		case SEND:
		{
		ret=OnSend(trans_bytes);
		}break;
		case SHUTDOWN:
		{
		ret=OnShutDown(trans_bytes);
		}break;
		case CONNECT:
		{
		ret=OnConnect(trans_bytes);
		}break;
		}
		return ret;*/
	}
	int IOCP_Socket::IOFail()
	{
		ErrorMessage(::WSAGetLastError(),L"ICOP_Socket",L"IO Fail");
		//SetError();
		ShutDown();
		return 0;
	}
	bool IOCP_Socket::TimeCheck(DWORD now)
	{
		if(!this->SocketClosed())
		{
			DWORD span=now-actionTick;
			if(actionTick>0 && span>timeoutSpan)
			{
				SetError();
				CloseSocket();
				closeTime=now;
			}
		}
		else if(now - closeTime>30*1000)
			return false;
		return true;
	}
	bool IOCP_Socket::StartConnect(const struct sockaddr* name,
		int namelen,PVOID lpSendBuffer,DWORD dwSendDataLength)
	{
		if(IOCP_Shell::fn_connectex==nullptr) return false;
		memcpy_s(&addr,sizeof(addr),name,namelen);
		//state=CONNECT;
		SetNextIOFunction(&IOCP_Socket::OnConnect);
		SetActionTick();
		if(FALSE==IOCP_Shell::fn_connectex(s,name,namelen,lpSendBuffer,dwSendDataLength,
			&transed,this->GetZeroOV()))
		{
			DWORD lastError=WSAGetLastError();
			if(WSA_IO_PENDING!=lastError)
			{
				ErrorMessage(lastError,L"ICOP_Socket",L"Connect error");
				SetError();
				return false;
			}
		}
		return true;
	}
	bool IOCP_Socket::StartRecv(ULONG recvsize)
	{
		if(recvsize==-1)
			wbuf.len=buffer.GetCount();
		else
			wbuf.len=recvsize;
		//state=RECV;
		SetNextIOFunction(&IOCP_Socket::OnRecv);
		SetActionTick();
		if(SOCKET_ERROR==WSARecv(s,&wbuf,1,&transed,&flag,this->GetZeroOV(),0))
		{
			DWORD lastError=WSAGetLastError();
			if(WSA_IO_PENDING!=lastError)
			{
				ErrorMessage(lastError,L"ICOP_Socket",L"Recv error");
				SetError();
				return false;
			}
		}

		return true;
	}
	bool IOCP_Socket::StartRecv(WSABUF* bufs,DWORD bufcount)
	{
		//state=RECV;
		SetNextIOFunction(&IOCP_Socket::OnRecv);
		SetActionTick();
		if(SOCKET_ERROR==WSARecv(s,bufs,bufcount,&transed,&flag,this->GetZeroOV(),0))
		{
			DWORD lastError=WSAGetLastError();
			if(WSA_IO_PENDING!=lastError)
			{
				ErrorMessage(lastError,L"ICOP_Socket",L"Recv error");
				SetError();
				return false;
			}
		}

		return true;
	}
	bool IOCP_Socket::StartSend(DWORD size)
	{
		if(size!=-1)
			wbuf.len=size;
		//state=SEND;
		SetNextIOFunction(&IOCP_Socket::OnSend);
		SetActionTick();
		if(SOCKET_ERROR==WSASend(s,&wbuf,1,&transed,0,this->GetZeroOV(),0))
		{
			DWORD lastError=WSAGetLastError();
			if(WSA_IO_PENDING!=lastError)
			{
				ErrorMessage(lastError,L"ICOP_Socket",L"Send error");
				SetError();
				return false;
			}
		}

		return true;
	}
	bool IOCP_Socket::StartSend(WSABUF* bufs,DWORD bufcount)
	{
		//state=SEND;
		SetNextIOFunction(&IOCP_Socket::OnSend);
		SetActionTick();
		if(SOCKET_ERROR==WSASend(s,bufs,bufcount,&transed,0,this->GetZeroOV(),0))
		{
			DWORD lastError=WSAGetLastError();
			if(WSA_IO_PENDING!=lastError)
			{
				ErrorMessage(lastError,L"ICOP_Socket",L"Send error");
				SetError();
				return false;
			}
		}
		return true;
	}
	bool IOCP_Socket::WriteSendPack(char *buf,int sz)
	{
		if(!AllocBuffer(sizeof(int)+sz))
			return false;
		memcpy(buffer.GetData(),&sz,sizeof(int));
		memcpy(buffer.GetData()+sizeof(int),buf,sz);
		return StartSend(sizeof(int)+sz);
	}

	void InitAcceptSocket(SOCKET s)
	{
		int optval=1;
		//int ooptv=0;
		int res;
		//res=setsockopt(s,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&optval,sizeof(optval));  //禁止端口多用
		res=setsockopt(s,SOL_SOCKET,SO_KEEPALIVE,(char*)&optval,sizeof(optval));         //发送保持连接
		//res=setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char*)&ooptv,sizeof(ooptv));              //设置发送缓冲区
		//res=setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char*)&ooptv,sizeof(ooptv));              //设置接收缓冲区
		//res=setsockopt(s,IPPROTO_TCP,TCP_NODELAY,(char*)&optval,sizeof(optval));         //不采用延时算
		//res=setsockopt(s,SOL_SOCKET,SO_CONDITIONAL_ACCEPT,(char*)&optval,sizeof(optval));//延迟连接响应
		res=setsockopt(s,SOL_SOCKET,SO_DONTLINGER,(char*)&optval,sizeof(optval));        //执行立即关闭
		linger lng;
		lng.l_linger=0;
		lng.l_onoff=1;
		res=setsockopt(s,SOL_SOCKET,SO_LINGER,(char*)&lng,sizeof(lng));
	}

	SocketSyncAccept::SocketSyncAccept(CreateSocketFunc csfunc,CanAcceptNow canaccept):
	csfunc(csfunc),
		canaccept(canaccept),
		accepter(INVALID_SOCKET),accepted(INVALID_SOCKET)
	{

	}
	SocketSyncAccept::~SocketSyncAccept()
	{
		Stop();
	}
	bool SocketSyncAccept::Start(LPCWSTR port,int af)
	{
		ATLASSERT(af==AF_INET || af==AF_INET6);
		this->af=af;
		if(port)
			listenport=port;
		if(listenport.IsEmpty()) return false;
		ADDRINFOW hint,*addrres=nullptr;
		ZeroMemory(&hint,sizeof(hint));
		hint.ai_flags=AI_PASSIVE;
		hint.ai_family=af;
		hint.ai_socktype=SOCK_STREAM;
		hint.ai_protocol=IPPROTO_TCP;
		if(0!=GetAddrInfoW(nullptr,listenport,&hint,&addrres))
		{
			SetError();
			return false;
		}

		accepter=WSASocket(addrres->ai_family,addrres->ai_socktype,addrres->ai_protocol,
			nullptr,0,WSA_FLAG_OVERLAPPED);
		int res=bind(accepter,addrres->ai_addr,addrres->ai_addrlen);
		FreeAddrInfoW(addrres);
		if(res==SOCKET_ERROR)
		{
			closesocket(accepter);
			accepter=INVALID_SOCKET;
			SetError();
			return false;
		}
		res=listen(accepter,1);
		if(res==SOCKET_ERROR)
		{
			closesocket(accepter);
			accepter=INVALID_SOCKET;
			SetError();
			return false;
		}

		if(!BindIOCP(accepter))
		{
			closesocket(accepter);
			accepter=INVALID_SOCKET;
			SetError();
			return false;
		}

		return AcceptNext();
	}
	void SocketSyncAccept::Stop()
	{
		if(INVALID_SOCKET!=accepter)
			closesocket(accepter);
		if(INVALID_SOCKET!=accepted)
			closesocket(accepted);
		accepter=accepted=INVALID_SOCKET;
	}
	bool SocketSyncAccept::AcceptNext()
	{
		if(accepted!=INVALID_SOCKET) return true;
		CAutoCTryLock al(lock);
		if(al.IsLocked()==false) return false;
		if(accepted!=INVALID_SOCKET) return true;
		if(this->canaccept)
			if(!this->canaccept()) return true;

		accepted=CreateTcpSocketFromPool(af);
		if(accepted==SOCKET_ERROR) return false;
		if(!AcceptEx(accepter,accepted,&acceptbuf,0,sizeof(acceptbuf)/2,sizeof(acceptbuf)/2,&transed,this->GetZeroOV()))
		{
			DWORD error=WSAGetLastError();
			if(ERROR_IO_PENDING!=error)
			{
				closesocket(accepted);
				accepted=INVALID_SOCKET;
				ErrorMessage(error,L"AcceptNext",L"accept error");
				return false;
			}
		}
		return true;
	}
	int SocketSyncAccept::IOComplete(DWORD trans_bytes)
	{
		this->transed=trans_bytes;

		CIPtr<IOCP_Socket> p_new_s=this->csfunc();
		sockaddr *premote,*plocal;
		int remotelen,locallen;
		GetAcceptExSockaddrs(&acceptbuf,0,sizeof(acceptbuf)/2,sizeof(acceptbuf)/2,
			&plocal,&locallen,&premote,&remotelen);
		p_new_s->InitSocket(accepted,premote,remotelen);
		p_new_s->SetSocketOpt(SOL_SOCKET, 
			SO_UPDATE_ACCEPT_CONTEXT, 
			(char *)&accepter, 
			sizeof(accepter));

		accepted=INVALID_SOCKET;
	
		if(p_new_s->RunFirstAction()==false)
		{
			ErrorMessage(::GetLastError(),L"SocketSyncAccept",L"new Socket First Action Fail.");
			IOCP_Shell::RemoveItem(p_new_s->GetOV());
		}
		AcceptNext();
		return 0;
	}
	int SocketSyncAccept::IOFail()
	{
		//SetError();
		if(accepted!=INVALID_SOCKET)
		{
			closesocket(accepted);
			accepted=INVALID_SOCKET;
			ErrorMessage(::GetLastError(),L"SocketSyncAccept",L"IO Fail");
		}
		return 0;
	}
	bool SocketSyncAccept::TimeCheck(DWORD /*now*/)
	{
		if(accepted==INVALID_SOCKET)
		{
			AcceptNext();
		}
		return true;
	}



	vector<SOCKET> TcpSocketPool::socketlist;
	CCriticalLock TcpSocketPool::lock;
	vector<SOCKET> TcpSocketPool::socketlist6;
	CCriticalLock TcpSocketPool::lock6;
	SOCKET TcpSocketPool::GetSocket(int af)
	{
		switch(af)
		{
		case AF_INET:
			{
				if(socketlist.empty()) return INVALID_SOCKET;
				CAutoCLock al(lock);
				SOCKET temp=socketlist.back();
				socketlist.pop_back();
				return temp;
			}break;
		case AF_INET6:
			{
				if(socketlist6.empty()) return INVALID_SOCKET;
				CAutoCLock al(lock6);
				SOCKET temp=socketlist6.back();
				socketlist6.pop_back();
				return temp;
			}break;
		default:
			{
				return INVALID_SOCKET;
			}break;
		}
	}
	void TcpSocketPool::ReleaseSocket(SOCKET s,int af)
	{
		switch(af)
		{
		case AF_INET:
			{
				CAutoCLock al(lock);
				socketlist.push_back(s);
			}break;
		case AF_INET6:
			{
				CAutoCLock al(lock6);
				socketlist6.push_back(s);
			}break;
		default:
			closesocket(s);
			break;
		}
	}

	SOCKET CreateTcpSocketFromPool(int af)
	{
		SOCKET socket=INVALID_SOCKET;
		socket=TcpSocketPool::GetSocket(af);
		if(socket==INVALID_SOCKET)
		{
			socket=WSASocket(af,SOCK_STREAM,IPPROTO_TCP,nullptr,0,WSA_FLAG_OVERLAPPED);
		}
		return socket;
	}
}


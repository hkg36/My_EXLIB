#include "SocketFrm.h"

CSyncSocket::CSyncSocket():busy(false)
{
}
void CSyncSocket::Init(const SOCKET s)
{
	this->m_s=s;
	m_haserror=false;
}
void CSyncSocket::Close()
{
	if(m_s!=INVALID_SOCKET)
	{
		if(SOCKET_ERROR==closesocket(m_s))
		{
			if(WSAEWOULDBLOCK==::WSAGetLastError())
			{
				return;
			}
		}
		m_s=INVALID_SOCKET;
	}
}
CStringA CSyncSocket::GetAddrStr(const sockaddr* addr,int addrlen)const
{
	if(addr==nullptr)
	{
		addr=&m_addr;
		addrlen=sizeof(m_addr);
	}
	CStringA res;
	char hostname[128];
	char servInfo[NI_MAXSERV];
	if(0==getnameinfo(addr,addrlen,
		hostname,_countof(hostname),
		servInfo,NI_MAXSERV,
		NI_NUMERICSERV|NI_NUMERICHOST))
	{
		res.Format("%s:%s",hostname,servInfo);
	}
	return res;
}
CSyncSocket::~CSyncSocket()
{
	Close();
}
void CSyncSocket::OnClose()
{
	SetError();
}
bool CSyncSocket::Bind(sockaddr* local,int locallen)
{
	SetAddr(local,locallen);
	return bind(m_s,local,locallen)==0;
}
bool CSyncSocket::Listen()
{
	return listen(m_s,10)==0;
}
CIPtr<CSyncSocket> CSyncSocket::Accept(CIPtr<CSyncSocket> emptysocket)
{
	sockaddrstore remote;
	int remlen=sizeof(remote);
	SOCKET new_s=WSAAccept(m_s,(sockaddr*)&remote,&remlen,ConditionFunc,(DWORD_PTR)this);
	if(new_s!=INVALID_SOCKET)
	{
		emptysocket->Init(new_s);
		emptysocket->SetAddr(&remote,remlen);
		CSocketFrame::GetFramWindow()->AddSocket(emptysocket);
		return emptysocket;
	}
	return nullptr;
}
bool CSyncSocket::Connect(const sockaddr* remote,int remotelen)
{
	if(SOCKET_ERROR==connect(GetSocket(),remote,remotelen))
	{
		if(WSAGetLastError()==WSAEWOULDBLOCK)
			return true;
		else
		{
			CStringW temp;
			temp.Format(L"connect ´íÎó:%d",WSAGetLastError());
			OutPutInfo(temp);
			return false;
		}
	}
	else
		return true;
}
bool CSyncSocket::Connect(LPCWSTR host,LPCWSTR port)
{
	ADDRINFOW hint,*resaddr=nullptr;
	ZeroMemory(&hint,sizeof(hint));
	hint.ai_family=AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	int res=GetAddrInfoW(host,port,&hint,&resaddr);
	if(res==0)
	{
		bool connres=false;
		if(resaddr)
		{

			if(resaddr->ai_addrlen>=1)
			{
				connres=Connect(resaddr->ai_addr,resaddr->ai_addrlen);
			}
			FreeAddrInfoW(resaddr);
		}
		return connres;
	}
	else
	{
		return false;
	}
}
bool CSyncSocket::WriteWrap(const char* start,int len)
{
	ATLASSERT(start!=nullptr && len!=0);
	m_buffer.WriteData(start,len);
	SendBufferData();
	return !IsError();
}
int CSyncSocket::Read(char *buffer,int bufsz)
{
	int res=recv(GetSocket(),buffer,bufsz,0);
	return res;
}
void CSyncSocket::SendBufferData()
{
	if(busy) return;
	while(true)
	{
		char *buftosend=nullptr;
		int size=0;
		if(m_buffer.WantRead(&buftosend,4096,&size))
		{
			int res=send(GetSocket(),buftosend,size,0);
			if(res!=SOCKET_ERROR)
			{
				m_buffer.ReadMoveNext(res);
			}
			else
			{
				if(WSAEWOULDBLOCK==WSAGetLastError())
				{
					busy=true;
					return;
				}
				else
				{
					SetError();
					return;
				}
			}
		}
		else
		{
			OnWriteBufferFinish();
			return;
		}
	}
}
void CSyncSocket::OnWrite()
{
	busy=false;
	SendBufferData();
}
CIPtr<CSyncSocket> CreateSocket(int af,
						  int type,
						  int protocol,
						  LPWSAPROTOCOL_INFO lpProtocolInfo,
						  GROUP g,
						  DWORD dwFlags,
						  CIPtr<CSyncSocket> emptysocket)
{
	if(emptysocket==nullptr) return nullptr;
	SOCKET s=WSASocket(af,type,protocol,lpProtocolInfo,g,dwFlags);
	if(s==INVALID_SOCKET)
	{
		ATLTRACE("SOCKET ´´½¨Ê§°Ü %d %d %d\n",af,type,protocol);
		return nullptr;
	}
	emptysocket->Init(s);
	if(CSocketFrame::GetFramWindow()->AddSocket(emptysocket))
		return emptysocket;
	else
		return nullptr;
}
CSocketFrame *CSocketFrame::onlyone=nullptr;

bool CSocketFrame::AddSocket(LPSyncSocket newsocket)
{
	if(0!=WSAAsyncSelect(newsocket->GetSocket(),*this,WM_SOCKET_EVENT,
		FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE|FD_QOS
		|FD_GROUP_QOS|FD_ROUTING_INTERFACE_CHANGE|FD_ADDRESS_LIST_CHANGE))
	{
		return false;
	}
	socketmap.SetAt(newsocket->GetSocket(),newsocket);
	return true;
}
void CSocketFrame::RemoveSocket(SOCKET s)
{
	socketmap.RemoveKey(s);
}

LRESULT CSocketFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ATLASSERT(onlyone==nullptr);
	onlyone=this;
	// register object for message filtering and idle updates

	SetTimer(SOCKET_CHECK,100,0);
	return 0;
}

LRESULT CSocketFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// unregister message filtering and idle updates
	onlyone=nullptr;
	socketmap.RemoveAll();
	bHandled = FALSE;
	return 1;
}

void CSocketFrame::OnTimer(UINT_PTR id)
{
	switch(id)
	{
	case SOCKET_CHECK:
		{
			DWORD now=::GetTickCount();
			POSITION pos=socketmap.GetHeadPosition();
			while(pos)
			{
				POSITION oldpos=pos;
				auto *one=socketmap.GetNext(pos);
				if(one->m_value->IsError())
				{
					socketmap.RemoveAt(oldpos);
					continue;
				}
				one->m_value->OnTimer(now);
				if(one->m_value->IsError())
					socketmap.RemoveAt(oldpos);
			}
		}break;
	}
}

LRESULT CSocketFrame::OnSocketEvent(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	SOCKET sock=(SOCKET)wParam;
	DWORD socket_error=WSAGETSELECTERROR(lParam);
	DWORD socket_event=WSAGETSELECTEVENT(lParam);
	auto *pos=socketmap.Lookup(sock);
	if(pos)
	{
		if(socket_error==0)
		{
			switch(socket_event)
			{
			case FD_READ:
				pos->m_value->OnRead();
				break;
			case FD_WRITE:
				pos->m_value->OnWrite();
				break;
			case FD_OOB:
				pos->m_value->OnOOB();
				break;
			case FD_ACCEPT:
				pos->m_value->OnAccept();
				break;
			case FD_CONNECT:
				pos->m_value->OnConnect();
				break;
			case FD_CLOSE:
				pos->m_value->OnClose();
				break;
			case FD_QOS:
				pos->m_value->OnQOS();
				break;
			case FD_GROUP_QOS:
				pos->m_value->OnGroupQOS();
				break;
			case FD_ROUTING_INTERFACE_CHANGE:
				pos->m_value->OnRoutingInterfaceChange();
				break;
			case FD_ADDRESS_LIST_CHANGE:
				pos->m_value->OnAddressListChange();
				break;
			}
		}
		else
		{
			pos->m_value->OnError(socket_error,socket_event);
		}

		if(pos->m_value->IsError())
			socketmap.RemoveKey(sock);
	}
	return 0;
}
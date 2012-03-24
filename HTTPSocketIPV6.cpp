#include "HttpClient.h"
#include <Ws2tcpip.h>
namespace HTTPClient
{
	SocketIPV6::SocketIPV6():s(INVALID_SOCKET)
	{
		WSADATA wsadata;
		WSAStartup(MAKEWORD(2,2),&wsadata);
	}
	int SocketIPV6::SetAddress(LPCWSTR host,LPCWSTR port)
	{
		int res=0;
		ADDRINFOW hints;
		ZeroMemory(&hints,sizeof(hints));
		hints.ai_family=AF_UNSPEC;
		hints.ai_protocol=IPPROTO_TCP;
		hints.ai_socktype=SOCK_STREAM;
		PADDRINFOW resinfo=nullptr;
		PADDRINFOW reswalker=nullptr;
		res=GetAddrInfoW(host,port,&hints,&resinfo);
		if(0!=res) return res;
		if(resinfo==nullptr) return res;
		addrlist.clear();
		reswalker=resinfo;
		while(reswalker)
		{
			sockaddrstore oneaddr;
			memcpy_s(&oneaddr,sizeof(oneaddr),resinfo->ai_addr,resinfo->ai_addrlen);
			addrlist.push_back(oneaddr);
			reswalker=reswalker->ai_next;
		}
		FreeAddrInfoW(resinfo);
		return res;
	}
	int SocketIPV6::Connect(size_t addr_id)
	{
		Close();
		if(addr_id>=addrlist.size()) return SOCKET_ERROR;
		s=socket(addrlist[addr_id].sa_family,SOCK_STREAM,IPPROTO_TCP);
		if(s!=INVALID_SOCKET)
		{
			int tout=1000;
			int res=setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tout,sizeof(tout));
			res=setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&tout,sizeof(tout));
			return connect(s,&addrlist[addr_id],sizeof(sockaddrstore));
		}
		else return SOCKET_ERROR;
	}
	void SocketIPV6::Close()
	{
		if(s!=INVALID_SOCKET)
		{
			closesocket(s);
			s=INVALID_SOCKET;
		}
	}
	SocketIPV6::~SocketIPV6()
	{
		Close();
		WSACleanup();
	}
	int SocketIPV6::Send(const char* buf,int bufsize)
	{
		if(s==INVALID_SOCKET)
			return SOCKET_ERROR;
		return send(s,buf,bufsize,0);
	}
	int SocketIPV6::Recv(char* buf,int bufsize)
	{
		if(s==INVALID_SOCKET)
			return SOCKET_ERROR;
		return recv(s,buf,bufsize,0);
	}
}
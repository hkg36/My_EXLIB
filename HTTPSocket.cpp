#include "HttpClient.h"
namespace HTTPClient
{
	Socket::Socket():s(INVALID_SOCKET)
	{
		ZeroMemory(&addr,sizeof(addr));
		addr.sin_family=AF_INET;
		WSADATA wsadata;
		WSAStartup(MAKEWORD(2,2),&wsadata);
	}
	int Socket::Connect(CAtlStringA hostname,int port)
	{
		Close();
		hostent* he=gethostbyname(hostname);
		if(he)
		{
			if(he->h_length>=4)
			{
				addr.sin_addr.s_addr=*(LONG*)(he->h_addr_list[0]);
			}
		}
		else
		{
			addr.sin_addr.s_addr=inet_addr(hostname);
		}
		addr.sin_port=htons(port);

		s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(s!=INVALID_SOCKET)
		{
			int tout=1000;
			int res=setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tout,sizeof(tout));
			res=setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&tout,sizeof(tout));
			return connect(s,(const sockaddr*)&addr,sizeof(addr));
		}
		else return SOCKET_ERROR;
	}
	int Socket::Connect(const in_addr host,int port)
	{
		addr.sin_addr=host;
		addr.sin_port=htons(port);
		s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(s!=INVALID_SOCKET)
		{
			int tout=1000;
			int res=setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tout,sizeof(tout));
			res=setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&tout,sizeof(tout));
			return connect(s,(const sockaddr*)&addr,sizeof(addr));
		}
		else return SOCKET_ERROR;
	}
	void Socket::Close()
	{
		if(s!=INVALID_SOCKET)
		{
			closesocket(s);
			s=INVALID_SOCKET;
		}
	}
	Socket::Socket(IN_ADDR host,int port):s(INVALID_SOCKET)
	{
		ZeroMemory(&addr,sizeof(addr));
		addr.sin_family=AF_INET;
		WSADATA wsadata;
		WSAStartup(MAKEWORD(2,2),&wsadata);
		addr.sin_addr=host;
		addr.sin_port=htons(port);
	}

	Socket::~Socket()
	{
		Close();
		WSACleanup();
	}

	int Socket::Send(const char* buf,int bufsize)
	{
		if(s==INVALID_SOCKET)
			return SOCKET_ERROR;
		return send(s,buf,bufsize,0);
	}
	int Socket::Recv(char* buf,int bufsize)
	{
		if(s==INVALID_SOCKET)
			return SOCKET_ERROR;
		return recv(s,buf,bufsize,0);
	}


	SocketEx::SocketEx():s(INVALID_SOCKET)
	{
		WSADATA wsadata;
		WSAStartup(MAKEWORD(2,2),&wsadata);
	}
	int SocketEx::Connect(const sockaddr* addr,int addrlen)
	{
		s=socket(addr->sa_family,SOCK_STREAM,IPPROTO_TCP);
		if(s!=INVALID_SOCKET)
		{
			int tout=120*1000;
			int res=setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tout,sizeof(tout));
			res=setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&tout,sizeof(tout));
			return connect(s,addr,addrlen);
		}
		else return SOCKET_ERROR;
	}
	void SocketEx::Close()
	{
		if(s!=INVALID_SOCKET)
		{
			closesocket(s);
			s=INVALID_SOCKET;
		}
	}
	SocketEx::~SocketEx()
	{
		Close();
		WSACleanup();
	}
	int SocketEx::Send(const char* buf,int bufsize)
	{
		if(s==INVALID_SOCKET)
			return SOCKET_ERROR;
		return send(s,buf,bufsize,0);
	}
	int SocketEx::Recv(char* buf,int bufsize)
	{
		if(s==INVALID_SOCKET)
			return SOCKET_ERROR;
		return recv(s,buf,bufsize,0);
	}
}
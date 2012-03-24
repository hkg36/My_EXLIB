#pragma once
#include "cexarray.h"
#include <atlcoll.h>
#include "ByteStream.h"
namespace HTTPClient
{
	class IHttpSocket
	{
	public:
		virtual int Send(const char* buf,int bufsize)=0;
		virtual int Recv(char* buf,int bufsize)=0;
	};
	class Socket:public IHttpSocket
	{
	private:
		CAtlStringA host;
		SOCKET s;
		sockaddr_in addr;
	public:
		Socket();
		int Connect(CAtlStringA hostname,int port=80);
		int Connect(const in_addr,int port=80);
		void Close();
		Socket(IN_ADDR host,int port=80);
		~Socket();
		int Send(const char* buf,int bufsize);
		int Recv(char* buf,int bufsize);
	};
	//和extools.h的CSockAddr 配合取得 sockaddr 就能兼容 ipv4和ipv6并且支持域名解析和机器名解析.
	class SocketEx:public IHttpSocket
	{
	private:
		SOCKET s;
	public:
		SocketEx();
		int Connect(const sockaddr* addr,int addrlen);
		void Close();
		~SocketEx();
		int Send(const char* buf,int bufsize);
		int Recv(char* buf,int bufsize);
	};
	class SocketIPV6:public IHttpSocket
	{
	private:
		SOCKET s;
		vector<sockaddrstore> addrlist;
	public:
		size_t AddrCount(){return addrlist.size();}
		SocketIPV6();
		int SetAddress(LPCWSTR host,LPCWSTR port=L"80");
		int Connect(size_t addr_id=0);
		void Close();
		~SocketIPV6();
		int Send(const char* buf,int bufsize);
		int Recv(char* buf,int bufsize);
	};
	class HttpMultipartPostBody;
	class HttpRequest
	{
	public:
		typedef map<CAtlStringA,CAtlStringA,StrLessNoCase> HeadMap;
	private:
		CAtlStringA Type;
		CAtlStringA Uri;
		CAtlStringA body;

		HeadMap headmap;

		void Init(const CAtlStringA type,const CAtlStringA uri);
	public:
		HttpRequest(const CAtlStringA type,const CAtlStringA uri);
		HttpRequest(const CAtlStringA uri="/");
		void SetUri(const CAtlStringA uri);
		void SetType(const CAtlStringA Type){this->Type=Type;}
		void RemoveHead(const CAtlStringA head);
		CAtlStringA setHead(const CAtlStringA head,const CAtlStringA value);
		CAtlStringA getHost();
		CAtlStringA setHost(const CAtlStringA host);
		CAtlStringA setReferer(const CAtlStringA ref);
		CAtlStringA setAccept(const CAtlStringA acp);
		CAtlStringA setUserAgent(const CAtlStringA ua);
		CAtlStringA setConnection(const CAtlStringA cn);
		CAtlStringA setRange(const unsigned __int64 start,const unsigned __int64 end=0);
		CAtlStringA setAcceptEncoding(const CAtlStringA ec);
		CAtlStringA setContentType(const CAtlStringA ct);
		CAtlStringA setContentLength(const unsigned int cl);
		CAtlStringA setBody(const CAtlStringA pb);
		void setPostBody(const CAtlStringA newbody);//限body是参数表使用
		void setPostBody(const CAtlStringW newbody);//限body是参数表使用，自动编码成UTF-8
		CAtlStringA BuildRequest();
		int SendRequest(IHttpSocket *s);
		int SendRequest(IHttpSocket *s,HttpMultipartPostBody* body);
	};
	class HttpResponse
	{
	private:
		CAtlStringA vision;
		int code;
		CAtlStringA resultexplain;
		HttpRequest::HeadMap headmap;
		void setHead(const CAtlStringA head,const CAtlStringA value);
		bool ReadEntityHeader(IHttpSocket *s);
		CAtlStringA ReadLine(IHttpSocket *s);
		bool DecodeHeadSection(CAtlStringA line);
	public:
		const CAtlStringA getVision();
		const int getCode()const;
		const CAtlStringA getResultExplain()const;
		const CAtlStringA getHead(const CAtlStringA head) const;
		const __int64 getContentLength() const;
		const CAtlStringA getContentType(CAtlStringA &subtype)const;
		const CAtlStringA getContentRange(__int64 *fullsize,__int64 *start,__int64 *end)const;
		const CAtlStringA getContentEncoding()const;
		const CAtlStringA getAcceptRanges()const;
		const CAtlStringA getTransferEncoding()const;
		bool RecvResponseHead(IHttpSocket *s);
		bool RecvResponseBody(IHttpSocket *s,CComPtr<IStream> body);
	};
	class HttpMultipartPostBody
	{
	private:
		CAtlStringA boundaryWord;
		struct PartInfo
		{
			CAtlStringA str;
			bool IsStream;
			CComPtr<IStream> stream;
		};
		typedef map<CAtlStringA,PartInfo,StrLessNoCase > PartList;
		PartList partlist;
		deque<PartInfo> outputpart;
		ULONGLONG outputpartsize;
	public:
		HttpMultipartPostBody();
		CAtlStringA GetBoundary(){return boundaryWord;}
		void Clear();
		void AddPart(CAtlStringA name,CAtlStringA value);
		void AddPart(CAtlStringA name,CAtlStringA value,CComPtr<IStream> stream);
		ULONGLONG BuiltSendPart();
		ULONGLONG SendRequestBody(IHttpSocket *s);
	};
	class HttpAsyncResponse
	{
	private:
		CAtlStringA bufferline;
		CAtlStringA vision;
		DWORD code;
		CAtlStringA info;
		map<CAtlStringA,CAtlStringA,StrLessNoCase > headlinemap;
		CComPtr<IStream> recvbuf;
		CComPtr<IMemoryStream> defaultbuf;
		DWORD state;
		DWORD torecvbody;
		bool result;
		enum{RecvFirstLine,RecvHeadLine,RecvBody,RecvChunkHead,RecvChunkBody,RecvChunkBodyTail,RecvChunkTailHeadLine};
		bool RecvOneByte(const char one);
	public:
		HttpAsyncResponse();
		CAtlStringA GetHead(CAtlStringA name) const
		{
			auto i=headlinemap.find(name);
			if(i==headlinemap.end())
				return CAtlStringA();
			else
				return i->second;
		}
		CAtlStringA GetVision()const{return vision;}
		DWORD GetCode()const{return code;}
		CAtlStringA GetInfo()const{return info;}
		IMemoryStream* GetDefaultBuffer(){return defaultbuf;}
		IStream* GetRecvBuffer(){return recvbuf;}
		bool GetResult()const{return result;}
		void Init(CComPtr<IStream> buf=nullptr);
		bool ProcessBuffer(char *buffer,size_t size,size_t &proced);
	};
}
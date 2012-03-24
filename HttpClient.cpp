#include "HttpClient.h"
#include "UnZipStreams.h"
#include "Random32.h"
namespace HTTPClient
{
	CAtlStringA HttpEncodeUri(CAtlStringA srcuri)
	{
		CAtlStringA res;
		LPCSTR point=srcuri;
		while(*point)
		{
			if((*point&(0x1<<8))==0)
			{
				if(*point!=' ')
					res.AppendChar(*point);
				else
					res.Append("%20");
				point++;
			}
			else
			{
				unsigned char first=*point;
				unsigned char second=*(point+1);
				res.AppendFormat("%%%02X%%%02X",first,second);
				point+=2;
			}
		}
		return res;
	}
	CAtlStringA HttpDecodeUri(CAtlStringA srcuri)
	{
		CAtlStringA res;
		LPCSTR point=srcuri;
		while(*point)
		{
			if(*point!='%')
			{
				res.AppendChar(*point);
				point++;
			}
			else
			{
				unsigned int word;
				if(1==sscanf_s(point,"%%%02X",&word,1))
					res.AppendChar(word);
				point+=3;
			}
		}
		return res;
	}
	void HttpRequest::Init(const CAtlStringA type,const CAtlStringA uri)
	{
		Type=type;
		SetUri(uri);
		setAccept("*/*");
		setUserAgent("Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)");
		setConnection("Keep-Alive");
		setAcceptEncoding("gzip,deflate");
	}
	HttpRequest::HttpRequest(const CAtlStringA type,const CAtlStringA uri)
	{
		Init(type,uri);
	}
	HttpRequest::HttpRequest(const CAtlStringA uri)
	{
		Init("GET",uri);
	}
	void HttpRequest::SetUri(const CAtlStringA uri)
	{
		Uri=HttpEncodeUri(uri);
	}
	void HttpRequest::RemoveHead(const CAtlStringA head)
	{
		headmap.erase(head);
	}
	CAtlStringA HttpRequest::setHead(const CAtlStringA head,const CAtlStringA value)
	{
		CAtlStringA oldvalue=headmap[head];
		if(value.IsEmpty())
			headmap.erase(head);
		else
			headmap[head]=value;
		return oldvalue;
	}
	CAtlStringA HttpRequest::getHost()
	{
		const static CAtlStringA key="HOST";
		HeadMap::iterator i=headmap.find(key);
		if(i!=headmap.end())
		{
			return i->second;
		}
		return "";
	}
	CAtlStringA HttpRequest::setHost(const CAtlStringA host)
	{
		const static CAtlStringA key="HOST";
		return setHead(key,host);
	}
	CAtlStringA HttpRequest::setReferer(const CAtlStringA ref)
	{
		const static CAtlStringA key="Referer";
		return setHead(key,ref);
	}
	CAtlStringA HttpRequest::setAccept(const CAtlStringA acp)
	{
		const static CAtlStringA key="Accept";
		return setHead(key,acp);
	}
	CAtlStringA HttpRequest::setUserAgent(const CAtlStringA ua)
	{
		const static CAtlStringA key="User-Agent";
		return setHead(key,ua);
	}
	CAtlStringA HttpRequest::setConnection(const CAtlStringA cn)
	{
		const static CAtlStringA key="Connection";
		return setHead(key,cn);
	}
	CAtlStringA HttpRequest::setRange(const unsigned __int64 start,const unsigned __int64 end)
	{
		CAtlStringA temp;
		if(end!=0 && start<end)
		{
			temp.Format("bytes=%I64u-%I64u",start,end);
		}
		else if(start!=0 && end==0)
		{
			temp.Format("bytes=%I64u-",start);
		}

		const static CAtlStringA key="Range";
		if(!temp.IsEmpty())
			return setHead(key,temp);
		else
			return temp;
	}
	CAtlStringA HttpRequest::setAcceptEncoding(const CAtlStringA ec)
	{
		const static CAtlStringA key="Accept-Encoding";
		return setHead(key,ec);
	}
	CAtlStringA HttpRequest::setContentType(const CAtlStringA ct)
	{
		const static CAtlStringA key="Content-Type";
		return setHead(key,ct);
	}
	CAtlStringA HttpRequest::setContentLength(const unsigned int cl)
	{
		CAtlStringA temp;
		temp.Format("%u",cl);
		const static CAtlStringA key="Content-Length";
		return setHead(key,temp);
	}
	CAtlStringA HttpRequest::setBody(const CAtlStringA pb)
	{
		CAtlStringA oldbody=body;
		body=pb;
		setContentLength(pb.GetLength());
		return oldbody;
	}
	void HttpRequest::setPostBody(const CAtlStringA newbody)
	{
		setBody(HttpEncodeUri(newbody));
		Type="POST";
		this->setContentType("application/x-www-form-urlencoded");
	}
	void HttpRequest::setPostBody(const CAtlStringW newbody)
	{
		CAtlStringA utf8body=CW2A(newbody,CP_UTF8);
		setBody(HttpEncodeUri(utf8body));
		Type="POST";
		this->setContentType("application/x-www-form-urlencoded;charset=utf-8");
	}
	CAtlStringA HttpRequest::BuildRequest()
	{
		CAtlStringA sendstr;
		sendstr.AppendFormat("%s %s HTTP/1.1\r\n",Type,Uri);
		for(HeadMap::iterator i=headmap.begin();i!=headmap.end();i++)
		{
			sendstr.AppendFormat("%s:%s\r\n",i->first,i->second);
		}
		sendstr.Append("\r\n");
		if(Type.CompareNoCase("POST")==0 && body.IsEmpty()==FALSE)
		{
			sendstr.Append(body);
		}
		return sendstr;
	}
	int HttpRequest::SendRequest(IHttpSocket *s)
	{
		CAtlStringA sendstr=BuildRequest();
		int tosend=sendstr.GetLength();
		if(SOCKET_ERROR==s->Send(sendstr.GetBuffer(),tosend))
			return SOCKET_ERROR;
		return sendstr.GetLength();
	}
	int HttpRequest::SendRequest(IHttpSocket *s,HttpMultipartPostBody* body)
	{
		Type="POST";
		CAtlStringA sendstr;
		ULONGLONG bodylen=body->BuiltSendPart();
		setContentLength((unsigned int)bodylen);
		CStringA str;
		str.Format("multipart/form-data;boundary=%s",body->GetBoundary());
		//Content-Type:multipart/form-data;boundary=---------------------------7d33a816d302b6
		setContentType(str);

		sendstr.AppendFormat("%s %s HTTP/1.1\r\n",Type,Uri);
		for(HeadMap::iterator i=headmap.begin();i!=headmap.end();i++)
		{
			sendstr.AppendFormat("%s:%s\r\n",i->first,i->second);
		}
		sendstr.Append("\r\n");

		int sended=0,resend;
		resend=s->Send(sendstr.GetBuffer(),sendstr.GetLength());
		if(SOCKET_ERROR==resend)
			return SOCKET_ERROR;
		sended+=resend;

		resend=(int)body->SendRequestBody(s);
		if(SOCKET_ERROR==resend)
			return SOCKET_ERROR;
		sended+=resend;
		return sended;
	}

	HttpMultipartPostBody::HttpMultipartPostBody()
	{
		CRandom32 randomer;
		boundaryWord.AppendFormat("%X%X",randomer.Next(),randomer.Next());
	}
	void HttpMultipartPostBody::Clear()
	{
		partlist.clear();
	}
	void HttpMultipartPostBody::AddPart(CAtlStringA name,CAtlStringA value)
	{
		PartInfo info={value,false,nullptr};
		partlist[name]=info;
	}
	void HttpMultipartPostBody::AddPart(CAtlStringA name,CAtlStringA value,CComPtr<IStream> stream)
	{
		PartInfo info={value,true,stream};
		partlist[name]=info;
	}
	ULONGLONG HttpMultipartPostBody::BuiltSendPart()
	{
		outputpartsize=0;
		CAtlStringA builder;
		for(auto pos=partlist.begin();pos!=partlist.end();pos++)
		{
			if(pos->second.IsStream==false)
			{
				builder.AppendFormat("--%s\r\n",boundaryWord);
				builder.AppendFormat("Content-Disposition: form-data; name=\"%s\"\r\n\r\n",pos->first);
				builder.Append(pos->second.str);
				builder.Append("\r\n");
			}
			else
			{
				builder.AppendFormat("--%s\r\n",boundaryWord);
				builder.AppendFormat("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
					"Content-Type: application/octet-stream\r\n\r\n",pos->first,pos->second.str);
				PartInfo info={builder,false,nullptr};
				builder.Empty();
				outputpart.push_back(info);
				info.str.Empty();
				info.IsStream=true;
				info.stream=pos->second.stream;
				outputpart.push_back(info);
				builder.Append("\r\n");
			}
		}
		builder.AppendFormat("--%s--\r\n",boundaryWord);
		PartInfo info={builder,false,nullptr};
		outputpart.push_back(info);
		builder.Empty();

		//LARGE_INTEGER streampos={0,0};
		//ULARG_INTEGER respos;
		for(auto pos=outputpart.begin();pos!=outputpart.end();pos++)
		{
			if(pos->IsStream)
			{
				STATSTG stat;
				if(S_OK!=pos->stream->Stat(&stat,STATFLAG_NONAME))
				{
					return 0;
				}
				//one.stream->Seek(streampos,STREAM_SEEK_SET,&respos);
				outputpartsize+=stat.cbSize.QuadPart;
			}
			else
			{
				outputpartsize+=pos->str.GetLength();
			}
		}
		return outputpartsize;
	}
	ULONGLONG HttpMultipartPostBody::SendRequestBody(IHttpSocket *s)
	{
		ULONGLONG sendsize=0;
		LARGE_INTEGER streampos={0,0};
		ULARGE_INTEGER respos;
		for(auto pos=outputpart.begin();pos!=outputpart.end();pos++)
		{
			if(pos->IsStream)
			{
				pos->stream->Seek(streampos,STREAM_SEEK_SET,&respos);
				char buffer[1024];
				ULONG read=0;
				while(S_OK==pos->stream->Read(buffer,sizeof(buffer),&read))
				{
					if(read==0) break;
					if(SOCKET_ERROR==s->Send(buffer,read)) return 0;
					sendsize+=read;
				}
			}
			else
			{
				if(SOCKET_ERROR==s->Send((LPCSTR)pos->str,pos->str.GetLength())) return 0;
				sendsize+=pos->str.GetLength();
			}
		}
		return sendsize;
	}
}
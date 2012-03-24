#include "HttpServerResponse.h"

CHttpServerResponse::CHttpServerResponse(void)
{
}

CHttpServerResponse::~CHttpServerResponse(void)
{
}

void CHttpServerResponse::AddHead(const CAtlStringA key,const CAtlStringA value)
{
	headmap[key]=value;
}

CAtlStringA CHttpServerResponse::SaveHead()
{
	CAtlStringA savedhead;
	if(vision.IsEmpty() || code==0) 
		return savedhead;
	savedhead.AppendFormat("%s %d %s\r\n",vision,code,message);
	for(auto i=headmap.begin();
		i!=headmap.end();i++)
	{
		savedhead.AppendFormat("%s:%s\r\n",i->first,i->second);
	}
	savedhead.Append("\r\n");
	return savedhead;
}

void CHttpServerResponse::Init()
{
	vision.Empty();
	code=0;
	this->message.Empty();
	this->headmap.clear();
}

void CHttpServerResponse::Message(int code,const CAtlStringA msgstr)
{
	this->code=code;
	if(!msgstr.IsEmpty())
		message=msgstr;
	else
	{
		message=GetHttpCodeMessage(code);
	}
}

struct HttpResCode
{
	UINT code;
	LPCSTR msg;
};
HttpResCode rescode[]=
{
	{100,"Continue"},
	{101,"Switching Protocols"},
	{200,"OK"},
	{201,"Created"},
	{202,"Accepted"},
	{203,"Non-Authoritative Information"},
	{204,"No Content"},
	{205,"Reset Content"},
	{206,"Partial Content"},
	{300,"Multiple Choices"},
	{301,"Moved Permanently"},
	{302,"Found"},
	{303,"See Other"},
	{304,"Not Modified"},
	{305,"Use Proxy"},
	{307,"Temporary Redirect"},
	{400,"Bad Request"},
	{401,"Unauthorized"},
	{402,"Payment Required"},
	{403,"Forbidden"},
	{404,"Not Found"},
	{405,"Method Not Allowed"},
	{406,"Not Acceptable"},
	{407,"Proxy Authentication Required"},
	{408,"Request Time-out"},
	{409,"Conflict"},
	{410,"Gone"},
	{411,"Length Required"},
	{412,"Precondition Failed"},
	{413,"Request Entity Too Large"},
	{414,"Request-URI Too Large"},
	{415,"Unsupported Media Type"},
	{416,"Requested range not satisfiable"},
	{417,"Expectation Failed"},
	{500,"Internal Server Error"},
	{501,"Not Implemented"},
	{502,"Bad Gateway"},
	{503,"Service Unavailable"},
	{504,"Gateway Time-out"},
	{505,"HTTP Version not supported"}
};
struct HttpCodeLess
{
	bool operator()(const HttpResCode &a,const HttpResCode &b) const
	{
		return a.code<b.code;
	}
	bool operator()(const UINT a,const HttpResCode &b) const
	{
		return a<b.code;
	}
	bool operator()(const HttpResCode &a,const UINT b) const
	{
		return a.code<b;
	}
};
CAtlStringA CHttpServerResponse::GetHttpCodeMessage(UINT code)
{
	const HttpResCode* a=std::lower_bound(&rescode[0],&rescode[_countof(rescode)],code,HttpCodeLess());
	if(a!=&rescode[_countof(rescode)] && a->code==code)
	{
		return a->msg;
	}
	return "";
}
void CHttpServerResponse::setContentLength(__int64 v)
{
	CAtlStringA temp;
	temp.Format("%I64d",v);
	static const CAtlStringA key("Content-Length");
	headmap[key]=temp;
}
void CHttpServerResponse::setContentType(CAtlStringA maintype,CAtlStringA subtype)
{
	CAtlStringA temp;
	temp.Format("%s/%s",maintype,subtype);
	static const CAtlStringA key("Content-Type");
	headmap[key]=temp;
}
void CHttpServerResponse::setContentRange(__int64 fullsize,__int64 start,__int64 end)
{
	static const CAtlStringA key("Content-Range");
	if(start<0 && end<0 && fullsize>=0)
	{
		CAtlStringA temp;
		temp.Format("*/%I64d",fullsize);
		headmap[key]=temp;
	}
	else if(start>=0 && end>=0 && fullsize>=0)
	{
		CAtlStringA temp;
		temp.Format("%I64d-%I64d/%I64d",start,end,fullsize);
		headmap[key]=temp;
	}
	else
		ATLASSERT(false);
}
void CHttpServerResponse::setContentEncoding(CAtlStringA v)
{
	static const CAtlStringA key("Content-Encoding");
	headmap[key]=v;
}
void CHttpServerResponse::setAcceptRanges(CAtlStringA v)
{
	static const CAtlStringA key("Accept-Ranges");
	headmap[key]=v;
}
void CHttpServerResponse::setTransferEncoding(CAtlStringA v)
{
	static const CAtlStringA key("Transfer-Encoding");
	headmap[key]=v;
}
#include "HttpClient.h"
#include "UnZipStreams.h"
namespace HTTPClient
{
	void HttpResponse::setHead(const CAtlStringA head,const CAtlStringA value)
	{
		headmap[head]=value;
	}
	const CAtlStringA HttpResponse::getVision()
	{
		return vision;
	}
	const int HttpResponse::getCode()const
	{
		return code;
	}
	const CAtlStringA HttpResponse::getResultExplain()const
	{
		return resultexplain;
	}
	const CAtlStringA HttpResponse::getHead(const CAtlStringA head) const
	{
		HttpRequest::HeadMap::const_iterator i=headmap.find(head);
		if(i!=headmap.end())
			return i->second;
		else
			return "";
	}
	const __int64 HttpResponse::getContentLength() const
	{
		CAtlStringA res=getHead("Content-Length");
		if(res.IsEmpty()) return -1;
		return _atoi64(res);
	}
	const CAtlStringA HttpResponse::getContentType(CAtlStringA &subtype)const
	{
		CAtlStringA temp=getHead("Content-Type");
		int index=temp.Find('/');
		if(index!=-1)
		{
			subtype=temp.Mid(index+1);
			return temp.Left(index);
		}
		else
			return "";
	}
	const CAtlStringA HttpResponse::getContentRange(__int64 *fullsize,__int64 *start,__int64 *end)const
	{
		//"bytes %I64d-%I64d/%I64d"
		*fullsize=0;
		*start=0;
		*end=0;
		CAtlStringA temp=getHead("Content-Range");
		CAtlStringA type;
		CAtlStringA datal;
		int index=temp.Find(' ');
		if(index!=-1)
		{
			type=temp.Left(index);
			datal=temp.Mid(index+1);
			CAtlStringA left,right;
			index=datal.Find('/');
			if(index!=-1)
			{
				left=datal.Left(index);
				right=datal.Mid(index+1);
				if(!left.IsEmpty() && !right.IsEmpty())
				{
					if(left!="*")
						sscanf_s(left,"%I64d-%I64d",start,end);
					sscanf_s(right,"%I64d",fullsize);
				}
			}
		}
		else
			type=temp;
		return type;
	}
	const CAtlStringA HttpResponse::getContentEncoding()const
	{
		return getHead("Content-Encoding");
	}
	const CAtlStringA HttpResponse::getAcceptRanges()const
	{
		return getHead("Accept-Ranges");
	}
	const CAtlStringA HttpResponse::getTransferEncoding()const
	{
		return getHead("Transfer-Encoding");
	}
	CAtlStringA HttpResponse::ReadLine(IHttpSocket *s)
	{
		int res=0;
		int readZeroCount=0;
		CAtlStringA line;
		while(true)
		{
			char oneword;
			res=s->Recv(&oneword,1);
			if(res==1)
			{
				readZeroCount=0;
				if(oneword=='\n')
				{
					line.Trim(" \t\r\n");
					return line;
				}
				else
				{
					line+=oneword;
				}
			}
			else if(res==0)
			{
				readZeroCount++;
				if(readZeroCount>5)
					throw WSAGetLastError();
			}
			else if(res==SOCKET_ERROR)
			{
				int lasterrno=WSAGetLastError();
				throw lasterrno;
			}
		}
	}
	bool HttpResponse::DecodeHeadSection(CAtlStringA temp)
	{
		CAtlStringA left,right;
		int pos=temp.Find(':');
		if(pos!=-1)
		{
			left=temp.Left(pos);
			right=temp.Mid(pos+1);
			left.Trim();
			right.Trim();
			this->setHead(left,right);
			return true;
		}
		else
			return false;
	}
	bool HttpResponse::RecvResponseHead(IHttpSocket *s)
	{
		CAtlStringA line;
		CAtlList<CAtlStringA> lines; 
		while(true)
		{
			try
			{
				line=ReadLine(s);
				line.Trim(" \r\n\t");
			}
			catch(...)
			{
				return false;
			}
			if(line.IsEmpty())
			{
				if(!lines.IsEmpty())
					break;
			}
			else
			{
				lines.AddTail(line);
			}
		}

		code=0;
		int pos1,pos2;
		POSITION pos=lines.GetHeadPosition();

		CAtlStringA firstline=lines.GetNext(pos);
		pos1=firstline.Find(' ');
		pos2=firstline.Find(' ',pos1+1);
		vision=firstline.Left(pos1);
		if(pos2==-1)
		{
			code=atoi(firstline.Mid(pos1+1));
			resultexplain.Empty();
		}
		else
		{
			code=atoi(firstline.Mid(pos1+1,pos2-pos1-1));
			resultexplain=firstline.Mid(pos2+1);
		}

		while(pos)
		{
			firstline=lines.GetNext(pos);
			DecodeHeadSection(firstline);
		}
		return true;
	}
	bool HttpResponse::RecvResponseBody(IHttpSocket *s,CComPtr<IStream> body_buf)
	{
		LARGE_INTEGER pos;
		int res=0;
		__int64 start,end,fullsize;
		CAtlStringA type=this->getContentRange(&fullsize,&start,&end);
		__int64 contentsize=this->getContentLength();
		CAtlStringA encode=this->getContentEncoding();
		CAtlStringA tencode=this->getTransferEncoding();
		CAtlStringA maintype,subtype;
		maintype=this->getContentType(subtype);

		char buffer[1024];

		CComPtr<IStream> recvtemp;
		if(encode.CompareNoCase("gzip")==0)
		{
			if(S_OK!=CUnZipStream::CreateInstense(body_buf,&recvtemp)) return false;
		}
		else if(encode.CompareNoCase("deflate")==0)
		{
			if(S_OK!=CInflateStream::CreateInstense(body_buf,&recvtemp)) return false;
		}
		else
			recvtemp=body_buf;

		ULONG writen=0;
		HRESULT writeres;
		if(tencode.CompareNoCase("chunked")!=0)
		{
			while(contentsize)
			{
				res=s->Recv(buffer,sizeof(buffer));
				if(res==0 || res==SOCKET_ERROR)
				{
					if(contentsize>0)
					{
						writeres=WSAGetLastError();
						return false;
					}
					else
					{
						return true;
					}
				}
				char* tmpbuf=buffer;
				int towrite=res;
				contentsize-=res;
				while(towrite)
				{
					writeres=recvtemp->Write(tmpbuf,towrite,&writen);
					if(S_FALSE==writeres)
					{
						if(contentsize<0)
							return true;
					}
					if(writeres==S_OK)
					{
						towrite-=writen;
						tmpbuf+=writen;
					}
					else
						return false;
				}
			}
		}
		else
		{
			CAtlStringA thuncksizestr;
			char readbuf,lastread;
			int recvchunckcount=0;
			while(true)
			{
				DWORD chuncksize=0;
				thuncksizestr.Empty();
				while(true)
				{
					res=s->Recv(&readbuf,1);
					if(res!=1)
						return false;
					if(readbuf=='\n' && lastread=='\r')
						break;
					else if(readbuf!='\r')
						thuncksizestr+=readbuf;
					lastread=readbuf;
				}
				if(thuncksizestr.IsEmpty())
					return false;
				if(sscanf_s(thuncksizestr,"%x",&chuncksize)!=1)
					return false;
				if(chuncksize==0)
				{
					if(!ReadEntityHeader(s))return false;
					break;
				}
				else
				{
					ULONG toreadsize=chuncksize;
					while(toreadsize)
					{
						res=s->Recv(buffer,min(sizeof(buffer),toreadsize));
						if(res==SOCKET_ERROR) 
							return false;

						char* tmpbuf=buffer;
						int towrite=res;
						toreadsize-=res;
						while(towrite)
						{
							writeres=recvtemp->Write(tmpbuf,towrite,&writen);
							if(writeres==S_OK)
							{
								towrite-=writen;
								tmpbuf+=writen;
							}
							else
								return false;
						}
					}
					res=s->Recv(&lastread,1);
					if(res==SOCKET_ERROR) return false;
					res=s->Recv(&readbuf,1);
					if(res==SOCKET_ERROR) return false;
					if(readbuf!='\n' || lastread!='\r')
						return false;
				}
				recvchunckcount++;
			}
			pos.QuadPart=0;
		}
		return true;
	}
	bool HttpResponse::ReadEntityHeader(IHttpSocket *s)
	{
		CAtlStringA line;
		while(true)
		{
			try
			{
				line=ReadLine(s);
			}
			catch(...)
			{
				return false;
			}
			if(line.IsEmpty()) break;
			else
			{
				DecodeHeadSection(line);
			}
		}
		return true;
	}
}
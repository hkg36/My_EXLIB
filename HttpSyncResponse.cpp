#include "HttpClient.h"
#include "UnZipStreams.h"
namespace HTTPClient
{
	HttpAsyncResponse::HttpAsyncResponse()
	{
		ByteStream::CreateInstanse(&defaultbuf);
	}
	void HttpAsyncResponse::Init(CComPtr<IStream> buf)
	{
		if(buf==nullptr)
		{
			::ULARGE_INTEGER sz;
			sz.QuadPart=0;
			defaultbuf->SetSize(sz);
			recvbuf=defaultbuf;
		}
		else
			recvbuf=buf;
		bufferline.Empty();
		vision.Empty();
		code=(DWORD)-1;
		info.Empty();
		headlinemap.clear();
		state=RecvFirstLine;
		result=false;
		torecvbody=0;
	}
	bool HttpAsyncResponse::ProcessBuffer(char *buffer,size_t size,size_t &proced)
	{
		char *procbuf=buffer;
		proced=0;
		bool gorun=true;
		while(gorun)
		{
			gorun=false;
			if(state!=RecvBody && state!=RecvChunkBody)
			{
				while(true)
				{
					proced++;
					if(RecvOneByte(*procbuf)==false)
					{
						recvbuf->Commit(STGC_DEFAULT);
						return false;
					}
					if(proced>=size)
						return true;
					procbuf++;
					if(state==RecvBody || state==RecvChunkBody)
						break;
				}
			}
			
			if(state==RecvBody || state==RecvChunkBody)
			{
				if(torecvbody==0)
				{
					result=true;
					return false;
				}
				ULONG wrote;
				while(true)
				{
					ULONG towrite=min(size-proced,torecvbody);
					if(towrite==0)
						return true;
					if(S_OK!=recvbuf->Write(procbuf,towrite,&wrote))
					{
						return false;
					}
					procbuf+=wrote;
					proced+=wrote;
					torecvbody-=wrote;
					if(torecvbody==0)
					{
						if(state==RecvBody)
						{
							result=true;
							return false;
						}
						else if(state==RecvChunkBody)
						{
							gorun=true;
							state=RecvChunkBodyTail;
							break;
						}
					}
				}
			}
		}
		return false;
	}
	bool HttpAsyncResponse::RecvOneByte(const char one)
	{
		if(one=='\n')
		{
			bufferline.Trim(" \r\n\t");
		}
		else
		{
			bufferline.AppendChar(one);
			return true;
		}
		switch(state)
		{
		case RecvFirstLine:
			{
				if(bufferline.IsEmpty())return true;

				int pos=0,oldpos=0;
				pos=bufferline.Find(' ',oldpos);
				if(pos==-1) return false;
				vision=bufferline.Left(pos);

				oldpos=pos+1;
				pos=bufferline.Find(' ',oldpos);
				if(pos==-1)
				{
					code=atoi(bufferline.Mid(oldpos));
					state=RecvHeadLine;
					bufferline.Empty();
					return true;
				}
				else
				{
					code=atoi(bufferline.Mid(oldpos,pos-oldpos));
					oldpos=pos+1;
					info=bufferline.Mid(oldpos);
					state=RecvHeadLine;
					bufferline.Empty();
					return true;
				}
			}break;
		case RecvHeadLine:
		case RecvChunkTailHeadLine:
			{
				if(bufferline.IsEmpty())
				{
					if(state==RecvChunkTailHeadLine)
					{
						result=true;
						return false;
					}
					auto i=headlinemap.find("Content-Encoding");
					if(i!=headlinemap.end())
					{
						CComPtr<IStream> tmpstream=recvbuf;
						if(i->second.CompareNoCase("gzip")==0)
						{
							recvbuf=nullptr;
							if(S_OK!=CUnZipStream::CreateInstense(tmpstream,&recvbuf)) return false;
						}
						else if(i->second.CompareNoCase("deflate")==0)
						{
							recvbuf=nullptr;
							if(S_OK!=CInflateStream::CreateInstense(tmpstream,&recvbuf)) return false;
						}
					}
					i=headlinemap.find("Transfer-Encoding");
					if(i!=headlinemap.end())
					{
						if(i->second.CompareNoCase("chunked")==0)
						{
							state=RecvChunkHead;
							return true;
						}
					}
					i=headlinemap.find("Content-Length");
					if(i!=headlinemap.end())
					{
						torecvbody=atoi(i->second);
						if(torecvbody==0)
						{
							result=true;
							return false;
						}
						state=RecvBody;
						return true;
					}
					else
					{
						torecvbody=(DWORD)-1;
						state=RecvBody;
						return true;
					}
				}
				int pos=bufferline.Find(':');
				if(pos==-1) return false;
				CAtlStringA name,value;
				name=bufferline.Left(pos).Trim();
				value=bufferline.Mid(pos+1).Trim();
				if(name.IsEmpty() || value.IsEmpty()) return false;
				headlinemap[name]=value;
				bufferline.Empty();
				return true;
			}break;
		case RecvChunkHead:
			{
				DWORD chuncksize;
				if(sscanf_s(bufferline,"%x",&chuncksize)!=1) return false;
				bufferline.Empty();
				if(chuncksize==0)
				{
					result=true;
					state=RecvChunkTailHeadLine;
					return true;
				}
				torecvbody=chuncksize;
				state=RecvChunkBody;
				return true;
			}break;
		case RecvChunkBodyTail:
			{
				if(!bufferline.IsEmpty())
				{
					return false;
				}
				state=RecvChunkHead;
				return true;
			}break;
		}
		return false;
	}
}
#include "HttpServerRequest.h"

CAtlStringA CHttpServerRequest::HttpDecodeUri(CAtlStringA srcuri,DWORD trans)
{
	CAtlStringA res;
	LPCSTR point=srcuri;
	bool transed=false;
	while(*point)
	{
		if(*point!='%')
		{
			res.AppendChar(*point);
			point++;
		}
		else
		{
			transed=true;
			unsigned int word;
			if(1==sscanf_s(point,"%%%02X",&word,1))
				res.AppendChar(word);
			point+=3;
		}
	}
	if(transed)
	{
		CAtlStringW utf8dec=CA2W(res,trans);
		res=utf8dec;
	}
	return res;
}
CHttpServerRequest::CHttpServerRequest(void):resault(false)
{
	ByteStream::CreateInstanse(&bodystream);
	Init();
}

CHttpServerRequest::~CHttpServerRequest(void)
{
}
CAtlStringA CHttpServerRequest::GetHead(const CAtlStringA key) const
{
	auto i=headmap.find(key);
	if(i!=headmap.end())
		return i->second;
	return CAtlStringA();
}
bool CHttpServerRequest::getContentLength(int &value) const
{
	CAtlStringA res=GetHead(L"Content-Length");
	if(res.IsEmpty())
		return false;
	else
	{
		value=atoi(res);
		return true;
	}
}
void CHttpServerRequest::Init()
{
	resault=false;
	state=RecvFirstLine;
	act.Empty();
	uri.Empty();
	vision.Empty();
	headmap.clear();
	headlines.clear();
	inputbuffer.Empty();
	ULARGE_INTEGER sz;
	sz.QuadPart=0;
	bodystream->SetSize(sz);
}
bool CHttpServerRequest::InputBuffer(char* buf,int size,int &proced)
{
	bool res;
	char *procbuf=buf;
	proced=0;
	if(state!=RecvBody)
	{
		while(proced<size)
		{
			proced++;
			if(!(res=InputChar(*procbuf)))
				return false;
			procbuf++;
			if(state==RecvBody) break;
		}
	}
	if(state==RecvBody)
	{
		ULONG wrote;
		while(true)
		{
			ULONG towrite=min(size-proced,torecvbody);
			if(towrite==0)
				return true;
			if(S_OK!=bodystream->Write(procbuf,towrite,&wrote))
			{
				return false;
			}
			procbuf+=wrote;
			proced+=wrote;
			torecvbody-=wrote;
			if(torecvbody==0)
			{
				resault=true;
				return false;
			}
		}
	}
	return true;
}
bool CHttpServerRequest::InputChar(char one)
{
	switch(state)
	{
	case RecvFirstLine:
		{
			if(one!='\n')
			{
				if(inputbuffer.GetLength()>1024) return false;
				inputbuffer.AppendChar(one);
				return true;
			}
			inputbuffer.Trim("\r\n ");
			if(!inputbuffer.IsEmpty())
			{
				headlines.push_back(inputbuffer);
				inputbuffer.Empty();
				return true;
			}
			else
			{
				if(headlines.size()<2)
					return false;
				inputbuffer=headlines[0];

				int index=inputbuffer.Find(' ');
				if(index==-1) return false;
				act=inputbuffer.Mid(0,index);
				int indexold=index+1;
				index=inputbuffer.Find(' ',indexold);
				if(index==-1) return false;
				uri=inputbuffer.Mid(indexold,index-indexold);
				if(uri.IsEmpty())
					return false;
				uri=HttpDecodeUri(uri,CP_ACP);
				vision=inputbuffer.Mid(index+1);


				for(size_t i=1;i<headlines.size();i++)
				{
					inputbuffer=headlines[i];
					index=inputbuffer.Find(':');
					if(index==-1)
					{
						return false;
					}
					CAtlStringA first,second;
					first=inputbuffer.Mid(0,index);
					second=inputbuffer.Mid(index+1);
					first.Trim();
					second.Trim();
					headmap[first]=second;
				}
				if(act.CompareNoCase("GET")==0)
				{
					resault=true;
					return false;
				}
				else if(act.CompareNoCase("POST")==0)
				{
					if(getContentLength(torecvbody))
					{
						if(torecvbody)
						{
							state=RecvBody;
							return true;
						}
						else
						{
							resault=true;
							return false;
						}
					}
					else
					{
						resault=true;
						return false;
					}
				}
				else
				{
					return false;
				}
			}
		}break;
	}
	return true;
}

const CAtlStringA UriSplit::GetParam(const CAtlStringA name) const
{
	auto i=params.find(name);
	if(i==params.end())
		return CAtlStringA();
	return i->second;
}
bool UriSplit::ParamExist(const CAtlStringA name) const
{
	auto i=params.find(name);
	return i!=params.end();
}
bool UriSplit::Decode(LPCSTR str)
{
	path.clear();
	params.clear();
	fragment.Empty();

	if(*str!='/') return false;
	str++;
	CAtlStringA temp;
	while(true)
	{
		if(*str==0)
		{
			path.push_back(temp);
			temp.Empty();
			return true;
		}
		else if(*str=='?')
		{
			path.push_back(temp);
			temp.Empty();
			break;
		}
		else if(*str!='/')
		{
			temp.AppendChar(*str);
		}
		else
		{
			path.push_back(temp);
			temp.Empty();
		}
		str++;
	}
	str++;
	CAtlStringA value;
	enum{ReadName,ReadValue};
	int stat=ReadName;
	while(true)
	{
		if(*str==0)
		{
			if(stat==ReadName) return false;
			else
			{
				params[temp]=value;
				return true;
			}
		}
		if(stat==ReadName)
		{
			if(*str=='=')
			{
				stat=ReadValue;
			}
			else
			{
				temp.AppendChar(*str);
			}
		}
		else if(stat==ReadValue)
		{
			if(*str=='&' || *str=='#')
			{
				params[temp]=value;
				temp.Empty();
				value.Empty();
				if(*str=='#')
				{
					str++;
					break;
				}
				stat=ReadName;
			}
			else
			{
				if(*str=='%')
				{
					unsigned char tranc=0;
					str++;
					{
						switch(*str)
						{
						case '0':case '1':case '2':case '3':case '4':
						case '5':case '6':case '7':case '8':case '9':
							tranc|=*str-'0';
							break;
						case 'a':case 'A':tranc|=0xA;break;
						case 'b':case 'B':tranc|=0xB;break;
						case 'c':case 'C':tranc|=0xC;break;
						case 'd':case 'D':tranc|=0xD;break;
						case 'e':case 'E':tranc|=0xE;break;
						case 'f':case 'F':tranc|=0xF;break;
						default:
							return false;
						}
						tranc=tranc<<4;
						str++;
						switch(*str)
						{
						case '0':case '1':case '2':case '3':case '4':
						case '5':case '6':case '7':case '8':case '9':
							tranc|=*str-'0';
							break;
						case 'a':case 'A':tranc|=0xA;break;
						case 'b':case 'B':tranc|=0xB;break;
						case 'c':case 'C':tranc|=0xC;break;
						case 'd':case 'D':tranc|=0xD;break;
						case 'e':case 'E':tranc|=0xE;break;
						case 'f':case 'F':tranc|=0xF;break;
						default:
							return false;
						}
					}
					value.AppendChar(tranc);
				}
				else
					value.AppendChar(*str);
			}
		}
		str++;
	}
	while(*str!=0)
	{
		fragment.AppendChar(*str);
		str++;
	}
	return true;
}
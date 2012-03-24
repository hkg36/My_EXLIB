#include "HttpServerSocket.h"

CHttpServerSocket::CHttpServerSocket()
{
	ByteStream2::CreateInstanse(&memstream);
}
void CHttpServerSocket::PrepareForNextRequest()
{
	autoCloseTick=0;
	request.Init();
	response.Init();
	responsebody=memstream;
	ULARGE_INTEGER sz;
	sz.QuadPart=0;
	responsebody->SetSize(sz);
}
bool CHttpServerSocket::RunFirstAction()
{
	PrepareForNextRequest();
	if(this->AllocBuffer(1024)==nullptr)return false;
	return StartRecv(-1);
}
int CHttpServerSocket::OnRecv(DWORD trans_bytes)
{
	if(trans_bytes==0)
	{
		//SetError();
		ShutDown();
		return SOCKET_ERROR;
	}
	int proced;
	if(request.InputBuffer(this->GetBuffer(),trans_bytes,proced))
	{
		this->AllocBuffer(1024);
		this->StartRecv(1024);
		return 0;
	}
	else
	{
		contentbuffer.RemoveAll();
		if(proced<(int)trans_bytes)
		{
			int copysize=trans_bytes-proced;
			contentbuffer.SetCount(copysize);
			memcpy(contentbuffer.GetData(),buffer.GetData()+proced,copysize);
		}
	}
	return ProcessRequest();
}
int CHttpServerSocket::ProcessRequest()
{
	CAtlStringW temp;
	if(!request.Resault())
	{
		OutPutLog(L"请求无法解析",0,GetSockNameInfo());
		SetError();
		return SOCKET_ERROR;
	}
	if(!ProcessRequest(
		this->request,
		this->response,
		this->responsebody,
		this->bodytosend) )
	{
		SetError();
		return SOCKET_ERROR;
	}
	CAtlStringA headstr=response.SaveHead();
	if(headstr.IsEmpty())
	{
		SetError();
		return SOCKET_ERROR;
	}
	int strlen=headstr.GetLength();
	this->AllocBuffer(strlen);
	strncpy(this->GetBuffer(),headstr,strlen);
	temp.Format(L"%s -- %d -- body %I64u",CA2W(request.Uri()).m_psz,response.Code(),bodytosend);
	OutPutLog(temp,0,GetSockNameInfo());
	if(!StartSend(strlen))
	{
		temp.Format(L"send head fail %d",WSAGetLastError(),GetSockNameInfo());
		OutPutLog(temp);
		SetError();
	}
	return 0;
}
int CHttpServerSocket::OnSend(DWORD trans_bytes)
{
	if(trans_bytes==0)
	{
		ShutDown();
		return SOCKET_ERROR;
	}
	ATLASSERT(bodytosend>=0);
	if(bodytosend>0)
	{
		const int onebuffer=1024;
		if(!this->AllocBuffer(onebuffer))
		{
			SetError();
			return errno;
		}
		
		ULONG read;
		ULONG tosend=(ULONG)min((ULONGLONG)this->BufferSize(),bodytosend);
		if(S_OK==responsebody->Read(this->GetBuffer(),
			tosend,
			&read))
		{
			if(read==0)
			{
				SetError();
				return ::GetLastError();
			}
			bodytosend-=read;
			StartSend(read);
		}
		else
		{
			SetError();
			return ::GetLastError();
		}
	}
	else
	{
		return SendEnd_CheckNextAct();
	}
	return S_OK;
}
int CHttpServerSocket::SendEnd_CheckNextAct()
{
	CAtlStringA temp=request.GetHead("Connection");
	if(!temp.IsEmpty() && temp.CompareNoCase("Keep-Alive")!=0)
	{
		ShutDown();
		return 0;
	}
	else
	{
		temp=request.GetHead("Keep-Alive");
		if(!temp.IsEmpty() )
		{
			this->timeoutSpan=atoi(temp)*1000;
		}
	}
	PrepareForNextRequest();
	if(!contentbuffer.IsEmpty())
	{
		int proced;
		if(!request.InputBuffer(contentbuffer.GetData(),contentbuffer.GetCount(),proced))
		{
			contentbuffer.RemoveAt(0,proced);
			return ProcessRequest();
		}
		else
		{
			contentbuffer.RemoveAll();
		}
	}
	this->AllocBuffer(1024);
	StartRecv(1024);
	return 0;
}
int CHttpServerSocket::IOFail()
{
	DWORD error=WSAGetLastError();
	OutPutLog(L"io fail",error,GetSockNameInfo());
	return IOCP_Socket::IOFail();
}

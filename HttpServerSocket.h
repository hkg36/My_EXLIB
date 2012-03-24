#pragma once
#include "ByteStream2.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

class CHttpServerSocket:public IOCP::IOCP_Socket
{
protected:
	CHttpServerRequest request;
	CHttpServerResponse response;
	CComPtr<IMemoryStream2> memstream;
	CComPtr<IStream> responsebody;
	ULONGLONG bodytosend;
	DWORD autoCloseTick;
	CAtlArray<char> contentbuffer;
public:
	CHttpServerSocket();
	bool RunFirstAction();
	int OnRecv(DWORD trans_bytes);
	int OnSend(DWORD trans_bytes);
	int SendEnd_CheckNextAct();
	int IOFail();
	void PrepareForNextRequest();
	int ProcessRequest();
	virtual bool ProcessRequest(
		CHttpServerRequest &request,
		CHttpServerResponse &response,
		CComPtr<IStream> &responsebody,
		ULONGLONG &bodytosend)=0;
	virtual void OutPutLog(CAtlStringW msg,DWORD error=0,CAtlStringW exmsg=CAtlStringW())=0;
};

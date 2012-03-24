#include "SocketFrm.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "DataQueue.h"
class CSyncSocketHttp:public CSyncSocket
{
private:
	CHttpServerRequest request;
	CHttpServerResponse response;
public:
	void OnRead();
	virtual bool ProcessRequest(UriSplit &uri)=0;
};
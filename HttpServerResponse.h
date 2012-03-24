#pragma once
#include "cexarray.h"
class CHttpServerResponse
{
private:
	CAtlStringA vision;
	int code;
	CAtlStringA message;
	map<CAtlStringA,CAtlStringA,StrLessNoCase> headmap;
public:
	void Vision(const CAtlStringA value){vision=value;}
	void Message(int code,const CAtlStringA msgstr=CAtlStringA());
	void AddHead(const CAtlStringA key,const CAtlStringA value);
	CHttpServerResponse(void);
	~CHttpServerResponse(void);
	void Init();
	CAtlStringA SaveHead();
	int Code(){return code;}

	void setContentLength(__int64 v);
	void setContentType(CAtlStringA maintype,CAtlStringA subtype);
	void setContentRange(__int64 fullsize,__int64 start=-1,__int64 end=-1);
	void setContentEncoding(CAtlStringA v);
	void setAcceptRanges(CAtlStringA v="byte");
	void setTransferEncoding(CAtlStringA v);

	static CAtlStringA GetHttpCodeMessage(UINT code);
};

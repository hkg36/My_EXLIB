#pragma once
#include "IOCP.h"
#include "ByteStream.h"
class CHttpServerRequest
{
private:
	CAtlStringA act;
	CAtlStringA uri;
	CFixedStringT<CAtlStringA,256> vision;
	
	map<CAtlStringA,CAtlStringA,StrLessNoCase> headmap;
	CComPtr<IMemoryStream> bodystream;
	vector<CAtlStringA> headlines;

	CAtlStringA inputbuffer;
	enum{RecvFirstLine,RecvStatLine,RecvBody};
	int state;
	bool resault;
	
	int torecvbody;
public:
	CAtlStringA GetHead(const CAtlStringA key) const;
	bool getContentLength(int &value)const;
	bool Resault()const{return resault;}
	CAtlStringA Act()const{return act;}
	CAtlStringA Uri()const{return uri;}
	CAtlStringA Vision() const{return vision;}
	IStream* Body(){return bodystream;}
	CHttpServerRequest(void);
	~CHttpServerRequest(void);
	void Init();
	bool InputChar(char one);
	bool InputBuffer(char* buf,int size,int &proced);
	static CAtlStringA HttpDecodeUri(CAtlStringA srcuri,DWORD trans=CP_UTF8);
};

class UriSplit
{
private:
	vector<CAtlStringA> path;
	map<CAtlStringA,CAtlStringA,CString_Compare_NoCase > params;
	CAtlStringA fragment;
public:
	const CAtlStringA GetParam(const CAtlStringA name) const;
	bool ParamExist(const CAtlStringA name) const;
	const vector<CAtlStringA>& GetPath()const{return path;}
	bool Decode(LPCSTR str);
};
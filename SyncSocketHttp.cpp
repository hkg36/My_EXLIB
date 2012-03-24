#include "SyncSocketHttp.h"

void CSyncSocketHttp::OnRead()
{
	char buf[256];
	int recvsz=this->Read(buf,sizeof(buf));
	int proced=sizeof(buf);
	while(true)
	{
		if(proced<sizeof(buf))
		{
			int nowpos=proced;
			if(request.InputBuffer(buf+nowpos,recvsz-nowpos,proced))
			{
				return;
			}
			else
			{
				proced=nowpos+proced;
			}
		}
		else
		{
			if(request.InputBuffer(buf,recvsz,proced))
				return;
		}
		if(request.Resault()==false)
		{
			Close();
			return;
		}
		response.Init();
		response.Vision(request.Vision());
		UriSplit uri;
		if(uri.Decode(request.Uri())==false)
		{
			Close();
			return;
		}
		if(ProcessRequest(uri)==false)
		{
			response.Message(500);
			response.AddHead("Content-Length","0");
			CAtlStringA res=response.SaveHead();
			this->WriteWrap(res,res.GetLength());
		}
	}
}

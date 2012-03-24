#include "DataQueue.h"
#include <atlbase.h>
CDataQueue::DataBlock* CDataQueue::AllocBlock()
{
#ifdef _DEBUG
	memcount++;
#endif
	DataBlock* p=nullptr;
	p=(DataBlock*)::VirtualAlloc(nullptr,blocksize,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	::VirtualLock(p,blocksize);
	p->NextBlock=nullptr;
	return p;
}
void CDataQueue::ReleaseBlock(DataBlock* p)
{
#ifdef _DEBUG
	memcount--;
#endif
	::VirtualFree(p,blocksize,MEM_RELEASE);
}
CDataQueue::CDataQueue()
{
#ifdef _DEBUG
	memcount=0;
#endif
	SYSTEM_INFO sinfo;
	::GetSystemInfo(&sinfo);
	blocksize=sinfo.dwPageSize;
	head=tail=AllocBlock();
	nowreadpos=nowwritepos=0;
}
CDataQueue::~CDataQueue()
{
	DataBlock* p=head;
	while(p)
	{
		DataBlock* pnext=p->NextBlock;
		ReleaseBlock(p);
		p=pnext;
	}
#ifdef _DEBUG
	ATLASSERT(memcount==0);
#endif
}
void CDataQueue::WantWrite(char** buf,int wantwrite,int* canwrite)
{
	if(nowwritepos==blocksize-sizeof(DataBlock *))
	{
		DataBlock* newblock=AllocBlock();
		tail->NextBlock=newblock;
		tail=newblock;
		nowwritepos=0;
	}
	*canwrite=min(wantwrite,(int)(blocksize-sizeof(DataBlock *)-nowwritepos));
	*buf=&(tail->data[nowwritepos]);
}
void CDataQueue::WriteMoveNext(int move)
{
	nowwritepos+=move;
}
void CDataQueue::WriteData(const char* buffer,int buffersize)
{
	while(buffersize)
	{
		char* writepos;
		int towrite;
		WantWrite(&writepos,buffersize,&towrite);
		memcpy(writepos,buffer,towrite);
		buffer+=towrite;
		buffersize-=towrite;
		WriteMoveNext(towrite);
	}
}
bool CDataQueue::WantRead(char** buffer,int wantread,int *canread)
{
	if(head==tail && nowreadpos==nowwritepos) return false;
	if(head==tail)
	{
		*buffer=&(head->data[nowreadpos]);
		*canread=min(wantread,nowwritepos-nowreadpos);
	}
	else
	{
		*buffer=&(head->data[nowreadpos]);
		*canread=min(wantread,(int)(blocksize-sizeof(DataBlock *)-nowreadpos));
	}
	return true;
}
void CDataQueue::ReadMoveNext(int move)
{
	if(head==tail)
	{
		nowreadpos+=min(move,nowwritepos-nowreadpos);
	}
	else
	{
		nowreadpos+=min(move,(int)(blocksize-sizeof(DataBlock *)));
	}
	if(nowreadpos==blocksize-sizeof(DataBlock *))
	{
		if(head==tail)
		{
			nowreadpos=nowwritepos=0;
		}
		else
		{
			DataBlock* old=head;
			head=head->NextBlock;
			ReleaseBlock(old);
			nowreadpos=0;
		}
	}
}
bool CDataQueue::ReadData(char* buffer,int wantread,int* readed)
{
	*readed=0;
	while(wantread)
	{
		char* buf;
		int canread;
		if(!WantRead(&buf,wantread,&canread))
		{
			return true;
		}
		if(canread!=wantread)
		{
			int ss=0;
		}
		memcpy(buffer,buf,canread);
		*readed+=canread;
		wantread-=canread;
		buffer+=canread;
		ReadMoveNext(canread);
	}
	return true;
}

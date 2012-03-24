#pragma once
#include <windows.h>
class CDataQueue
{
private:
	struct DataBlock
	{
		DataBlock *NextBlock;
		char data[1];
	};
	DataBlock* head,*tail;
	int nowreadpos;
	int nowwritepos;
	DWORD blocksize;
#ifdef _DEBUG
	DWORD memcount;
#endif
	DataBlock* AllocBlock();
	void ReleaseBlock(DataBlock* p);
public:
	CDataQueue();
	~CDataQueue();
	void WantWrite(char** buf,int wantwrite,int* canwrite);
	void WriteMoveNext(int move);
	void WriteData(const char* buffer,int buffersize);
	bool WantRead(char** buffer,int wantread,int *canread);
	void ReadMoveNext(int move);
	bool ReadData(char* buffer,int wantread,int* readed);
};
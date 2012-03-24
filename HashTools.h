#pragma once

typedef UCHAR Sha1Value[20];
typedef UCHAR Md5Value[16];
class CSHA1
{
private:
	enum{SHA1HashSize=20};
	UINT Intermediate_Hash[SHA1HashSize/4];	// Message Digest  
	ULONGLONG Length;
	SHORT Message_Block_Index;
	UCHAR Message_Block[64];
	bool Computed;
	bool Corrupted;
	void SHA1PadMessage();
	void SHA1ProcessMessageBlock(const UCHAR *buffer);
public:
	void Reset();
	bool Input(const UCHAR *, unsigned int);
	bool Result(Sha1Value Message_Digest);
};

class CMD5
{
protected:
	UINT32 A;
	UINT32 B;
	UINT32 C;
	UINT32 D;
	void MD5_Init();
	ULONGLONG Length;
	UCHAR process_buffer[64];
	int process_buffer_index;
	bool Computed,Corrupted;
	void Hash_Round(const UCHAR *inputbuf);
	void PadRound();
public:
	void Reset();
	bool Input(const UCHAR * message_array, unsigned int length);
	bool Result(Md5Value Message_Digest);
};
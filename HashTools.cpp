#include <windows.h>
#include "HashTools.h"


// 32bit字左循环移位的宏
#define SHA1CircularShift(bits,word) \
	(((word) << (bits)) | ((word) >> (32-(bits))))
void CSHA1::Reset()
{
	Length             = 0;
	Message_Block_Index    = 0;

	Intermediate_Hash[0]   = 0x67452301;
	Intermediate_Hash[1]   = 0xEFCDAB89;
	Intermediate_Hash[2]   = 0x98BADCFE;
	Intermediate_Hash[3]   = 0x10325476;
	Intermediate_Hash[4]   = 0xC3D2E1F0;

	Computed   = 0;
	Corrupted  = 0;
}

//接收单位长度为8字节倍数的消息，处理最后一个分组前面的所有分组
bool CSHA1::Input(const UCHAR *message_array, unsigned length)
{
	if (!length)
	{
		return true;
	}

	if (!message_array)	
	{
		return false;
	}

	if (Computed)
	{
		Corrupted = true;
		return false;
	}

	if (Corrupted)
	{
		return Corrupted;
	}

	while(length && !Corrupted)
	{
		int copyed=min(sizeof(Message_Block)-Message_Block_Index,length);
		if(copyed==sizeof(Message_Block))
		{
			if(MAXULONGLONG-Length<sizeof(Message_Block)*8)
			{
				Corrupted = true;	// Message is too long
				break;
			}
			Length += sizeof(Message_Block)*8;
			SHA1ProcessMessageBlock(message_array);
			length-=sizeof(Message_Block);
			message_array+=sizeof(Message_Block);
			continue;
		}
		memcpy(Message_Block+Message_Block_Index,message_array,copyed);
		Message_Block_Index+=copyed;
		length-=copyed;
		message_array+=copyed;
		int copyedbit=copyed*8;
		if(MAXULONGLONG-Length<copyedbit)
		{
			Corrupted = true;	// Message is too long
			break;
		}
		Length += copyedbit;

		if (Message_Block_Index == 64)
		{
			SHA1ProcessMessageBlock(Message_Block);
		}
	}

	return true;
}

bool CSHA1::Result(Sha1Value Message_Digest)
{
	int i;

	if (!Message_Digest)	
	{
		return false;
	}

	if (Corrupted)
	{
		return false;
	}

	if (!Computed)	//执行此处
	{
		SHA1PadMessage();	//数据填充模块

		for(i=0; i<64; ++i)
		{
			Message_Block[i] = 0;	//消息清零 
		}

		Length = 0;	// 长度清零

		Computed = 1;
	}

	for(i = 0; i < SHA1HashSize; ++i)
	{
		//由Intermediate_Hash[0 ~ 4]得到Message_Digest[0 ~ 40]
		Message_Digest[i] = Intermediate_Hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) );
	}

	return true;
}

//数据填充模块，被SHA1Result调用
void CSHA1::SHA1PadMessage()
{
	if (Message_Block_Index > 55)
	{
		Message_Block[Message_Block_Index++] = 0x80;
		ZeroMemory(Message_Block+Message_Block_Index,64-Message_Block_Index);
		SHA1ProcessMessageBlock(Message_Block);
		ZeroMemory(Message_Block,56);
	}
	else
	{
		Message_Block[Message_Block_Index++] = 0x80;
		ZeroMemory(Message_Block+Message_Block_Index,56-Message_Block_Index);
	}

	//最后64位保存为数据长度
	Message_Block[56] =(UCHAR)(Length >> (24+32));
	Message_Block[57] =(UCHAR)(Length >> (16+32));
	Message_Block[58] =(UCHAR)(Length >> (8+32));
	Message_Block[59] =(UCHAR)(Length >> 32);
	Message_Block[60] =(UCHAR)(Length >> 24);
	Message_Block[61] =(UCHAR)(Length >> 16);
	Message_Block[62] =(UCHAR)(Length >> 8);
	Message_Block[63] =(UCHAR)(Length);

	SHA1ProcessMessageBlock(Message_Block);	//处理最后一个（含消息长度）512bit消息块
}

// 消息块（固定长度512bit）处理，被SHA1Input，SHA1PadMessage调用
void CSHA1::SHA1ProcessMessageBlock(const UCHAR *buffer)
{
	const UINT K[] = { 
		0x5A827999,
		0x6ED9EBA1,
		0x8F1BBCDC,
		0xCA62C1D6 
	};

	int           t;                  // 循环计数 
	UINT      temp;               // 临时缓存 
	UINT      W[80];              // 字顺序   
	UINT      A, B, C, D, E;      // 设置系统磁盘缓存块 

	//初始化W队列中的头16个字数据
	for(t = 0; t < 16; t++)
	{
		W[t] = buffer[t * 4] << 24;
		W[t] |= buffer[t * 4 + 1] << 16;
		W[t] |= buffer[t * 4 + 2] << 8;
		W[t] |= buffer[t * 4 + 3];
	}

	//字W[16]~W[79]的生成
	for(t = 16; t < 80; t++)
	{
		W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
	}

	A = Intermediate_Hash[0];
	B = Intermediate_Hash[1];
	C = Intermediate_Hash[2];
	D = Intermediate_Hash[3];
	E = Intermediate_Hash[4];

	// 定义算法所用之数学函数及其迭代算法描述
	for(t = 0; t < 20; t++)
	{
		temp =  SHA1CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for(t = 20; t < 40; t++)
	{
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for(t = 40; t < 60; t++)
	{
		temp = SHA1CircularShift(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	for(t = 60; t < 80; t++)
	{
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}

	// 迭代算法第80步（最后一步）描述
	Intermediate_Hash[0] += A;
	Intermediate_Hash[1] += B;
	Intermediate_Hash[2] += C;
	Intermediate_Hash[3] += D;
	Intermediate_Hash[4] += E;

	Message_Block_Index = 0;
}
struct MD5HelpFunc
{
	inline UINT32 F(UINT32 x,UINT32 y,UINT32 z)const{
		return (x&y)|((~x)&z);
	}
	inline UINT32 G(UINT32 x,UINT32 y,UINT32 z)const{
		return (x&z)|(y&(~z));
	}
	inline UINT32 H(UINT32 x,UINT32 y,UINT32 z)const{
		return x^y^z;
	}
	inline UINT32 I(UINT32 x,UINT32 y,UINT32 z)const{
		return y^(x|(~z));
	}

	inline void FF(UINT32 &a,UINT32 b,UINT32 c,UINT32 d,UINT32 mj,int s,UINT32 ti)const{
		a = a + F(b,c,d) + mj + ti;
		a = a << s | a >>(32-s);
		a += b;
	}
	inline void GG(UINT32 &a,UINT32 b,UINT32 c,UINT32 d,UINT32 mj,int s,UINT32 ti)const{
		a = a + G(b,c,d) + mj + ti;
		a = a << s | a >>(32-s);
		a += b;
	}
	inline void HH(UINT32 &a,UINT32 b,UINT32 c,UINT32 d,UINT32 mj,int s,UINT32 ti)const{
		a = a + H(b,c,d) + mj + ti;
		a = a << s | a >>(32-s);
		a += b;
	}
	inline void II(UINT32 &a,UINT32 b,UINT32 c,UINT32 d,UINT32 mj,int s,UINT32 ti)const{
		a = a + I(b,c,d) + mj + ti;
		a = a << s | a >>(32-s);
		a += b;
	}
};
void CMD5::MD5_Init(){
	Length=0;
	A=0x67452301; //in memory, this is 0x01234567
	B=0xefcdab89; //in memory, this is 0x89abcdef
	C=0x98badcfe; //in memory, this is 0xfedcba98
	D=0x10325476; //in memory, this is 0x76543210
	Computed=false;
	Corrupted=false;
}
void CMD5::Hash_Round(const UCHAR *inputbuf)
{
	UINT32 a;
	UINT32 b;
	UINT32 c;
	UINT32 d;

	/*
	unsigned int buffer[16];
	unsigned int i, j;
	for (i = 0, j = 0;i<16; i++, j += 4)
	{
		buffer[i] = ((unsigned int)((unsigned char)inputbuf[j])) |
			(((unsigned int)((unsigned char)inputbuf[j+1])) << 8) |
			(((unsigned int)((unsigned char)inputbuf[j+2])) << 16) |
			(((unsigned int)((unsigned char)inputbuf[j+3])) << 24);
	}*/
	unsigned int *buffer=(unsigned int *)inputbuf;
	enum{ S11=7,
		S12=12,
		S13=17,
		S14=22,
		S21=5,
		S22=9,
		S23=14,
		S24=20,
		S31=4,
		S32=11,
		S33=16,
		S34=23,
		S41=6,
		S42=10,
		S43=15,
		S44=21
	};
	a=A;
	b=B;
	c=C;
	d=D;
	MD5HelpFunc hf;
	hf.FF ( a, b, c, d, buffer[ 0], S11, 0xd76aa478); /* 1 */
	hf.FF ( d, a, b, c, buffer[ 1], S12, 0xe8c7b756); /* 2 */
	hf.FF ( c, d, a, b, buffer[ 2], S13, 0x242070db); /* 3 */
	hf.FF ( b, c, d, a, buffer[ 3], S14, 0xc1bdceee); /* 4 */
	hf.FF ( a, b, c, d, buffer[ 4], S11, 0xf57c0faf); /* 5 */
	hf.FF ( d, a, b, c, buffer[ 5], S12, 0x4787c62a); /* 6 */
	hf.FF ( c, d, a, b, buffer[ 6], S13, 0xa8304613); /* 7 */
	hf.FF ( b, c, d, a, buffer[ 7], S14, 0xfd469501); /* 8 */
	hf.FF ( a, b, c, d, buffer[ 8], S11, 0x698098d8); /* 9 */
	hf.FF ( d, a, b, c, buffer[ 9], S12, 0x8b44f7af); /* 10 */
	hf.FF ( c, d, a, b, buffer[10], S13, 0xffff5bb1); /* 11 */
	hf.FF ( b, c, d, a, buffer[11], S14, 0x895cd7be); /* 12 */
	hf.FF ( a, b, c, d, buffer[12], S11, 0x6b901122); /* 13 */
	hf.FF ( d, a, b, c, buffer[13], S12, 0xfd987193); /* 14 */
	hf.FF ( c, d, a, b, buffer[14], S13, 0xa679438e); /* 15 */
	hf.FF ( b, c, d, a, buffer[15], S14, 0x49b40821); /* 16 */
	/* Round 2 */
	hf.GG ( a, b, c, d, buffer[ 1], S21, 0xf61e2562); /* 17 */
	hf.GG ( d, a, b, c, buffer[ 6], S22, 0xc040b340); /* 18 */
	hf.GG ( c, d, a, b, buffer[11], S23, 0x265e5a51); /* 19 */
	hf.GG ( b, c, d, a, buffer[ 0], S24, 0xe9b6c7aa); /* 20 */
	hf.GG ( a, b, c, d, buffer[ 5], S21, 0xd62f105d); /* 21 */
	hf.GG ( d, a, b, c, buffer[10], S22, 0x2441453); /* 22 */
	hf.GG ( c, d, a, b, buffer[15], S23, 0xd8a1e681); /* 23 */
	hf.GG ( b, c, d, a, buffer[ 4], S24, 0xe7d3fbc8); /* 24 */
	hf.GG ( a, b, c, d, buffer[ 9], S21, 0x21e1cde6); /* 25 */
	hf.GG ( d, a, b, c, buffer[14], S22, 0xc33707d6); /* 26 */
	hf.GG ( c, d, a, b, buffer[ 3], S23, 0xf4d50d87); /* 27 */
	hf.GG ( b, c, d, a, buffer[ 8], S24, 0x455a14ed); /* 28 */
	hf.GG ( a, b, c, d, buffer[13], S21, 0xa9e3e905); /* 29 */
	hf.GG ( d, a, b, c, buffer[ 2], S22, 0xfcefa3f8); /* 30 */
	hf.GG ( c, d, a, b, buffer[ 7], S23, 0x676f02d9); /* 31 */
	hf.GG ( b, c, d, a, buffer[12], S24, 0x8d2a4c8a); /* 32 */
	/* Round 3 */
	hf.HH ( a, b, c, d, buffer[ 5], S31, 0xfffa3942); /* 33 */
	hf.HH ( d, a, b, c, buffer[ 8], S32, 0x8771f681); /* 34 */
	hf.HH ( c, d, a, b, buffer[11], S33, 0x6d9d6122); /* 35 */
	hf.HH ( b, c, d, a, buffer[14], S34, 0xfde5380c); /* 36 */
	hf.HH ( a, b, c, d, buffer[ 1], S31, 0xa4beea44); /* 37 */
	hf.HH ( d, a, b, c, buffer[ 4], S32, 0x4bdecfa9); /* 38 */
	hf.HH ( c, d, a, b, buffer[ 7], S33, 0xf6bb4b60); /* 39 */
	hf.HH ( b, c, d, a, buffer[10], S34, 0xbebfbc70); /* 40 */
	hf.HH ( a, b, c, d, buffer[13], S31, 0x289b7ec6); /* 41 */
	hf.HH ( d, a, b, c, buffer[ 0], S32, 0xeaa127fa); /* 42 */
	hf.HH ( c, d, a, b, buffer[ 3], S33, 0xd4ef3085); /* 43 */
	hf.HH ( b, c, d, a, buffer[ 6], S34, 0x4881d05); /* 44 */
	hf.HH ( a, b, c, d, buffer[ 9], S31, 0xd9d4d039); /* 45 */
	hf.HH ( d, a, b, c, buffer[12], S32, 0xe6db99e5); /* 46 */
	hf.HH ( c, d, a, b, buffer[15], S33, 0x1fa27cf8); /* 47 */
	hf.HH ( b, c, d, a, buffer[ 2], S34, 0xc4ac5665); /* 48 */
	/* Round 4 */
	hf.II ( a, b, c, d, buffer[ 0], S41, 0xf4292244); /* 49 */
	hf.II ( d, a, b, c, buffer[ 7], S42, 0x432aff97); /* 50 */
	hf.II ( c, d, a, b, buffer[14], S43, 0xab9423a7); /* 51 */
	hf.II ( b, c, d, a, buffer[ 5], S44, 0xfc93a039); /* 52 */
	hf.II ( a, b, c, d, buffer[12], S41, 0x655b59c3); /* 53 */
	hf.II ( d, a, b, c, buffer[ 3], S42, 0x8f0ccc92); /* 54 */
	hf.II ( c, d, a, b, buffer[10], S43, 0xffeff47d); /* 55 */
	hf.II ( b, c, d, a, buffer[ 1], S44, 0x85845dd1); /* 56 */
	hf.II ( a, b, c, d, buffer[ 8], S41, 0x6fa87e4f); /* 57 */
	hf.II ( d, a, b, c, buffer[15], S42, 0xfe2ce6e0); /* 58 */
	hf.II ( c, d, a, b, buffer[ 6], S43, 0xa3014314); /* 59 */
	hf.II ( b, c, d, a, buffer[13], S44, 0x4e0811a1); /* 60 */
	hf.II ( a, b, c, d, buffer[ 4], S41, 0xf7537e82); /* 61 */
	hf.II ( d, a, b, c, buffer[11], S42, 0xbd3af235); /* 62 */
	hf.II ( c, d, a, b, buffer[ 2], S43, 0x2ad7d2bb); /* 63 */
	hf.II ( b, c, d, a, buffer[ 9], S44, 0xeb86d391); /* 64 */
	A+=a;
	B+=b;
	C+=c;
	D+=d;
}

void CMD5::PadRound()
{
	if (process_buffer_index > 55)
	{
		process_buffer[process_buffer_index++] = 0x80;
		ZeroMemory(process_buffer+process_buffer_index,64-process_buffer_index);
		Hash_Round(process_buffer);
		process_buffer_index=0;
		ZeroMemory(process_buffer,56);
	}
	else
	{
		process_buffer[process_buffer_index++] = 0x80;
		ZeroMemory(process_buffer+process_buffer_index,56-process_buffer_index);
	}

	//最后64位保存为数据长度
	process_buffer[56] =(UCHAR)(Length);
	process_buffer[57] =(UCHAR)(Length>>8);
	process_buffer[58] =(UCHAR)(Length>>16);
	process_buffer[59] =(UCHAR)(Length>>24);
	process_buffer[60] =(UCHAR)(Length>>32);
	process_buffer[61] =(UCHAR)(Length>>40);
	process_buffer[62] =(UCHAR)(Length>>48);
	process_buffer[63] =(UCHAR)(Length>>56);

	Hash_Round(process_buffer);
}

void CMD5::Reset()
{
	MD5_Init();
	process_buffer_index=0;
}
bool CMD5::Input(const UCHAR * message_array, unsigned int length)
{
	if (!length)
	{
		return true;
	}

	if (!message_array)	
	{
		return false;
	}

	if (Computed)
	{
		Corrupted = true;
		return false;
	}

	if (Corrupted)
	{
		return false;
	}

	while(length)
	{
		int copyed=min(sizeof(process_buffer)-process_buffer_index,length);
		if(copyed==sizeof(process_buffer))
		{
			if(MAXULONGLONG-Length<sizeof(process_buffer)*8)
			{
				Corrupted = true;	// Message is too long
				break;
			}
			Length += sizeof(process_buffer)*8;
			Hash_Round(message_array);
			length-=sizeof(process_buffer);
			message_array+=sizeof(process_buffer);
			continue;
		}
		memcpy(process_buffer+process_buffer_index,message_array,copyed);
		process_buffer_index+=copyed;
		length-=copyed;
		message_array+=copyed;
		int copyedbit=copyed*8;
		if(MAXULONGLONG-Length<copyedbit)
		{
			Corrupted = true;	// Message is too long
			break;
		}
		Length += copyedbit;

		if (process_buffer_index == 64)
		{
			Hash_Round(process_buffer);
			process_buffer_index=0;
		}
	}
	return true;
}
bool CMD5::Result(Md5Value Message_Digest)
{
	if(Message_Digest==nullptr) return false;
	if(!Computed)
		PadRound();
	Message_Digest[0]=(UCHAR)A;
	Message_Digest[1]=(UCHAR)(A>>8);
	Message_Digest[2]=(UCHAR)(A>>16);
	Message_Digest[3]=(UCHAR)(A>>24);
	Message_Digest[4]=(UCHAR)B;
	Message_Digest[5]=(UCHAR)(B>>8);
	Message_Digest[6]=(UCHAR)(B>>16);
	Message_Digest[7]=(UCHAR)(B>>24);
	Message_Digest[8]=(UCHAR)C;
	Message_Digest[9]=(UCHAR)(C>>8);
	Message_Digest[10]=(UCHAR)(C>>16);
	Message_Digest[11]=(UCHAR)(C>>24);
	Message_Digest[12]=(UCHAR)(D);
	Message_Digest[13]=(UCHAR)(D>>8);
	Message_Digest[14]=(UCHAR)(D>>16);
	Message_Digest[15]=(UCHAR)(D>>24);
	Computed=true;
	return true;
}
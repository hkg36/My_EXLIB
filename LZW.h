#pragma once
#include <vector>

struct CompressStream
{
	int (*getc)(void* This);
	void (*putc)(void* This,int c);
};
//基本上只能用于数据库压缩存储，因为压缩解压速度快，其他应用建议用gzip
class CLZW
{
private:
#define BITS 12                   /* 设置每个常量的位数　*/
#define HASHING_SHIFT (BITS-8)      /* 为12，13或者为14*　 */
#define MAX_VALUE (1 << BITS) - 1 /* 当在MS-DOS下选择14位*/
#define MAX_CODE MAX_VALUE - 1    /* 时，提示需要重新编译*/

#if BITS == 14
	static const size_t TABLE_SIZE = 18041; 
#endif                           
#if BITS == 13          
	static const size_t TABLE_SIZE =9029;
#endif
#if BITS <= 12
	static const size_t TABLE_SIZE = 5021;
#endif
	int* code_value;                  /* 代码值数组        　　*/
	unsigned int* prefix_code;        /* 用于保存压缩前的数据　*/
	unsigned char* append_character;  /* 用于保存压缩后的数据  */
public:
	CLZW();
	~CLZW();
	int compress(CompressStream& input,CompressStream& output);
	int expand(CompressStream& input,CompressStream& output);
	void SetMallocFunction(void *(*malloc_fun)(size_t _Size),
		void  (*free_fun)(void * _Memory))
	{
		_malloc=malloc_fun;
		_free=free_fun;
	}
private:
	void *(*_malloc)(size_t _Size);
	void  (*_free)(void * _Memory);
	int find_match(int hash_prefix,unsigned int hash_character);
	unsigned char *decode_string(unsigned char *buffer,unsigned int code);
	int input_code(CompressStream& input);
	int output_code(CompressStream& output,unsigned int code);
};


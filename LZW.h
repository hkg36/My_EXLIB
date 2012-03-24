#pragma once
#include <vector>

struct CompressStream
{
	int (*getc)(void* This);
	void (*putc)(void* This,int c);
};
//������ֻ���������ݿ�ѹ���洢����Ϊѹ����ѹ�ٶȿ죬����Ӧ�ý�����gzip
class CLZW
{
private:
#define BITS 12                   /* ����ÿ��������λ����*/
#define HASHING_SHIFT (BITS-8)      /* Ϊ12��13����Ϊ14*�� */
#define MAX_VALUE (1 << BITS) - 1 /* ����MS-DOS��ѡ��14λ*/
#define MAX_CODE MAX_VALUE - 1    /* ʱ����ʾ��Ҫ���±���*/

#if BITS == 14
	static const size_t TABLE_SIZE = 18041; 
#endif                           
#if BITS == 13          
	static const size_t TABLE_SIZE =9029;
#endif
#if BITS <= 12
	static const size_t TABLE_SIZE = 5021;
#endif
	int* code_value;                  /* ����ֵ����        ����*/
	unsigned int* prefix_code;        /* ���ڱ���ѹ��ǰ�����ݡ�*/
	unsigned char* append_character;  /* ���ڱ���ѹ���������  */
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


#include "LZW.h"


CLZW::CLZW(void)
{
	code_value=nullptr;
	prefix_code=nullptr;
	append_character=nullptr;
	this->SetMallocFunction(malloc,free);
}
CLZW::~CLZW(void)
{
	if(code_value) _free(code_value);
	if(prefix_code) _free(prefix_code);
	if(append_character) _free(append_character);
}

int CLZW::compress(CompressStream& input,CompressStream& output)
{
	if(code_value==nullptr)
		code_value=(int*)_malloc(TABLE_SIZE*sizeof(int));
	if(prefix_code==nullptr)
		prefix_code=(unsigned int*)_malloc(TABLE_SIZE*sizeof(unsigned int));
	if(append_character==nullptr)
		append_character=(unsigned char*)_malloc(TABLE_SIZE*sizeof(unsigned char));
	unsigned int next_code;
	unsigned int character;
	unsigned int string_code;
	unsigned int index;
	int i;

	next_code=256;              
	for (i=0;i<TABLE_SIZE;i++)  /* �ڿ�ʼ֮ǰ�ȳ�ʼ���ַ����� */
		code_value[i]=-1;

	string_code=(input.getc)(&input);    /* ��ȡ���ݡ���������         */

	while ((character=(input.getc)(&input)) != (unsigned)EOF)
	{
		index=find_match(string_code,character);/* �鿴�ַ����Ƿ��Ѿ����뵽��*/
		if (code_value[index] != -1)            /* �У�����ǵĻ��ͻ�ȡ������*/
			string_code=code_value[index];        /* ������ǵĻ��ͼӵ�����ȥ��*/
		else                                   
		{                                    
			if (next_code <= MAX_CODE)
			{
				code_value[index]=next_code++;      //��256��ʼ�ӣ������ͼ�����ݳ�ͻ
				prefix_code[index]=string_code;
				append_character[index]=character;
			}
			output_code(output,string_code);  
			string_code=character;            
		}
	}
	/*
	** ѭ������
	*/
	output_code(output,string_code); /* ������һ������           */
	output_code(output,MAX_VALUE);   /* ��������е����һ������   */
	output_code(output,0);

	return 0;
}

/*
** ���ڲ���ƥ���prefix+char���ַ��б���
** ����ҵ��˷�������������Ҳ������ص�
** һ�����ַ������б�����������
*/
int CLZW::find_match(int hash_prefix,unsigned int hash_character)
{
	int index;
	int offset;

	index = (hash_character << HASHING_SHIFT) ^ hash_prefix;
	if (index == 0)
		offset = 1;
	else
		offset = TABLE_SIZE - index;
	while (1)
	{
		if (code_value[index] == -1)
			return(index);
		if (prefix_code[index] == hash_prefix && 
			append_character[index] == hash_character)
			return(index);
		index -= offset;
		if (index < 0)
			index += TABLE_SIZE;
	}
}

/*
**  ����ִ��LZW��ʽ���ļ�������ѹ��������ļ���
*/
int CLZW::expand(CompressStream& input,CompressStream& output)
{
	if(prefix_code==nullptr)
		prefix_code=(unsigned int*)_malloc(TABLE_SIZE*sizeof(unsigned int));
	if(append_character==nullptr)
		append_character=(unsigned char*)_malloc(TABLE_SIZE*sizeof(unsigned char));
	unsigned int next_code;
	unsigned int new_code;
	unsigned int old_code;
	int character;
	unsigned char *string;
	unsigned char decode_stack[4000]; /* ���ڱ����ѹ���������*/

	next_code=256;

	old_code=input_code(input);  /* �����һ�����룬����ʼ���ַ����� */
	character=old_code;          /* ����ַ�������ļ��ϡ����������� */
	(output.putc)(&output,old_code);          

	while ((new_code=input_code(input)) != (MAX_VALUE))
	{
		if (new_code>=next_code)
		{
			*decode_stack=character;
			string=decode_string(decode_stack+1,old_code);
		}
		else
			string=decode_string(decode_stack,new_code);
		/*
		** ������������ַ�����
		*/
		character=*string;
		while (string >= decode_stack)
			(output.putc)(&output,*string--);
		/*
		** ���ַ�����������µı���.
		*/
		if (next_code <= MAX_CODE)
		{
			prefix_code[next_code]=old_code;
			append_character[next_code]=character;
			next_code++;
		}
		old_code=new_code;
	}
	return 0;
}
/*
** ���ڴ��ַ����н��롣���ڻ���������
*/

unsigned char *CLZW::decode_string(unsigned char *buffer,unsigned int code)
{
	int i;

	i=0;
	while (code > 255)
	{
		*buffer++ = append_character[code];
		code=prefix_code[code];
		if (i++>=4094)
		{
			throw "Fatal error during code expansion.\n";
		}
	}
	*buffer=code;
	return(buffer);
}

int CLZW::input_code(CompressStream& input)
{
	unsigned int return_value;
	static int input_bit_count=0;
	static unsigned long input_bit_buffer=0L;

	while (input_bit_count <= 24)
	{
		input_bit_buffer |= 
			(unsigned long) (input.getc)(&input) << (24-input_bit_count);
		input_bit_count += 8;
	}
	return_value=input_bit_buffer >> (32-BITS);
	input_bit_buffer <<= BITS;
	input_bit_count -= BITS;
	return(return_value);
}

int CLZW::output_code(CompressStream& output,unsigned int code)
{
	static int output_bit_count=0;
	static unsigned long output_bit_buffer=0L;

	output_bit_buffer |= (unsigned long) code << (32-BITS-output_bit_count);
	output_bit_count += BITS;
	while (output_bit_count >= 8)
	{
		(output.putc)(&output,output_bit_buffer >> 24);
		output_bit_buffer <<= 8;
		output_bit_count -= 8;
	}
	return 0;
}
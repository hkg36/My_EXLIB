#pragma once
#include <atlbase.h>
#include <atlstr.h>
#include <atlfile.h>
struct OutputInterface
{
	void (*write)(OutputInterface* This,LPCSTR str,int strlen);
	void (*writeChar)(OutputInterface*This,char c);
	OutputInterface(void (*write)(OutputInterface* This,LPCSTR str,int strlen),void (*writeChar)(OutputInterface *This,char c)=WriteOneChar):
	write(write),writeChar(writeChar)
	{
	}
	void WriteCString(const CAtlStringA &str)
	{
		if(str.IsEmpty()==FALSE)
			write(this,str,str.GetLength());
	}
	static void WriteOneChar(OutputInterface*This,char c)
	{
		This->write(This,&c,1);
	}
};
struct StringOutput:public OutputInterface
{
	StringOutput():OutputInterface(WriteF,WriteCharF){}
	CAtlStringA ostr;
	static void WriteF(OutputInterface*This,LPCSTR str,int strlen)
	{
		((StringOutput*)This)->ostr.Append(str,strlen);
	}
	static void WriteCharF(OutputInterface*This,char c)
	{
		((StringOutput*)This)->ostr.AppendChar(c);
	}
};

class InputInterface
{
protected:
	char (*peekfun)(InputInterface* peek);
	char (*getfun)(InputInterface* peek);
public:
	inline char Get(){return getfun(this);}
	inline char Peek(){return peekfun(this);}
};
class InputStream:public InputInterface
{
public:
	InputStream(LPCSTR iStr,unsigned int Strlen) :
	  m_iStr(iStr),m_Strlen(Strlen),m_Location(0)
	  {
		  peekfun=rawPeek;
		  getfun=rawGet;
	  }

	  // protect access to the input stream, so we can keeep track of document/line offsets
#define This ((InputStream *)peek)
	  static char rawGet(InputInterface* peek)
	  {
		  if(This->m_Location>=This->m_Strlen) return 0;
		  char c = This->m_iStr[This->m_Location];
		  ++This->m_Location;
		  return c;
	  }
	  static char rawPeek(InputInterface* peek){
		  if(This->m_Location>=This->m_Strlen) return 0; 
		  return This->m_iStr[This->m_Location];
	  }
#undef This
private:
	LPCSTR m_iStr;
	unsigned int m_Strlen;
	unsigned int m_Location;
};
class FileInputStream:public InputInterface
{
private:
	CAtlFile file;
	char readbuf[128];
	DWORD readbufsz;
	char* readbufpos;
	bool iseos;
	inline void UpdateBuffer()
	{
		if(readbufpos==readbuf+readbufsz)
		{
			if(S_OK==file.Read(readbuf,sizeof(readbuf),readbufsz))
			{
				if(readbufsz<sizeof(readbuf))
					iseos=true;
				readbufpos=readbuf;
			}
		}
	}
public:
	FileInputStream(LPCTSTR filename)
	{
		file.Create(filename,FILE_READ_DATA,FILE_SHARE_READ,OPEN_EXISTING);
		peekfun=rawPeek;
		getfun=rawGet;
		readbufsz=0;
		readbufpos=readbuf;
		iseos=false;
	}
#define This ((FileInputStream *)peek)
	static char rawGet(InputInterface* peek)
	{
		if(This->iseos && This->readbufpos>=This->readbuf+This->readbufsz) return 0;
		This->UpdateBuffer();
		char c=*This->readbufpos;
		This->readbufpos++;
		return c;
	}
	static char rawPeek(InputInterface* peek){
		if(This->iseos && This->readbufpos>=This->readbuf+This->readbufsz) return 0;
		This->UpdateBuffer();
		return *This->readbufpos;
	}
#undef This
};
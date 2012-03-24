#pragma once
#ifdef _DEBUG

namespace Gdiplus
{
	namespace DllExports
	{
		#include <GdiplusMem.h>
	};

	#ifndef _GDIPLUSBASE_H
	#define _GDIPLUSBASE_H
	class GdiplusBase
	{
		public:
			void (operator delete)(void* in_pVoid)
			{
				DllExports::GdipFree(in_pVoid);
			}

			void* (operator new)(size_t in_size)
			{
				return DllExports::GdipAlloc(in_size);
			}

			void (operator delete[])(void* in_pVoid)
			{
				DllExports::GdipFree(in_pVoid);
			}

			void* (operator new[])(size_t in_size)
			{
				return DllExports::GdipAlloc(in_size);
			}

			void * (operator new)(size_t nSize, int,LPCSTR lpszFileName, int nLine)
			{
				return DllExports::GdipAlloc(nSize);
			}
			void* (operator new[])(size_t in_size,int,LPCSTR lpszFileName, int nLine)
			{
				return DllExports::GdipAlloc(in_size);
			}
		};
	#endif // #ifndef _GDIPLUSBASE_H
}
#endif // #ifdef _DEBUG
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
class GdiplusStarter
{
private:
	ULONG_PTR m_gdiplusToken;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
public:
	GdiplusStarter()
	{
		Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
	}
	~GdiplusStarter()
	{

		Gdiplus::GdiplusShutdown(m_gdiplusToken);
	}
};
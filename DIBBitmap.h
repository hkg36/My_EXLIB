#pragma once
#include <Windows.h>
#include <atlbase.h>
#include <atlimage.h>
class CDIBBitmap
{
	BYTE *bitmapData;
	int lineSize;
	size_t bmpDataSize;
	BITMAPINFO bitmapinfo;
public:
	CDIBBitmap()
	{
		bitmapData=NULL;
		bmpDataSize=0;
	}
	~CDIBBitmap()
	{
		if(bitmapData)
			free(bitmapData);
	}
	void SetBitmapSize(int width,int height)
	{
		ZeroMemory(&bitmapinfo,sizeof(bitmapinfo));
		bitmapinfo.bmiHeader.biSize=sizeof(bitmapinfo.bmiHeader);
		bitmapinfo.bmiHeader.biWidth=width;
		bitmapinfo.bmiHeader.biHeight=-height;
		bitmapinfo.bmiHeader.biPlanes=1;
		bitmapinfo.bmiHeader.biBitCount=24;
		bitmapinfo.bmiHeader.biCompression=BI_RGB;
		bitmapinfo.bmiHeader.biSizeImage=0;
		bitmapinfo.bmiHeader.biXPelsPerMeter=0;
		bitmapinfo.bmiHeader.biYPelsPerMeter=0;
		bitmapinfo.bmiHeader.biClrImportant=0;

		lineSize=(width*3+sizeof(DWORD)-1)/sizeof(DWORD)*sizeof(DWORD);
		size_t buffersize=lineSize*height;
		if(buffersize>bmpDataSize)
		{
			if(bitmapData)
				free(bitmapData);
			bitmapData=(unsigned char *)malloc(buffersize);
			bmpDataSize=buffersize;
		}
	}
	void SetPixel(int x,int y,BYTE R,BYTE G,BYTE B)
	{
		ATLASSERT(x<bitmapinfo.bmiHeader.biWidth);
		ATLASSERT(y<abs(bitmapinfo.bmiHeader.biHeight));

		BYTE *pos=(bitmapData+y*lineSize)+x*3;
		*pos=B;
		*(pos+1)=G;
		*(pos+2)=R;
	}
	void SetDIBitsToDevice(CDCHandle dc,int XDest,int YDest)
	{
		SetDIBitsToDevice(dc,XDest,YDest,bitmapinfo.bmiHeader.biWidth,abs(bitmapinfo.bmiHeader.biHeight));
	}
	void SetDIBitsToDevice(CDCHandle dc,int XDest,int YDest,DWORD dwWidth,DWORD dwHeight)
	{
		if(bitmapData)
		{
			int res=dc.SetDIBitsToDevice(XDest,YDest,dwWidth,dwHeight,0,0,0,abs(bitmapinfo.bmiHeader.biHeight),bitmapData,&bitmapinfo,DIB_RGB_COLORS);
			res=2;
		}
	}
};
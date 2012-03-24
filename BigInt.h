#pragma once
/*****************************************************************
大数运算库头文件：BigInt.h
作者：afanty@vip.sina.com
版本：1.2 (2003.5.13)
说明：适用于MFC，1024位RSA运算
*****************************************************************/
#include <atlbase.h>
#include <atlstr.h>
#include "Random32.h"
class CBigInt
{
	enum{DEC=10,HEX=16};
	friend class CPrimeCreator;
protected:
	//允许生成1120位（二进制）的中间结果
	enum{BI_MAXLEN=35};
	//大数在0x100000000进制下的长度    
	unsigned m_nLength;
	//用数组记录大数在0x100000000进制下每一位的值
	unsigned long m_ulValue[BI_MAXLEN];
public:
	CBigInt();
	~CBigInt();

	/*****************************************************************
	基本操作与运算
	Mov，赋值运算，可赋值为大数或普通整数，可重载为运算符“=”
	Cmp，比较运算，可重载为运算符“==”、“!=”、“>=”、“<=”等
	Add，加，求大数与大数或大数与普通整数的和，可重载为运算符“+”
	Sub，减，求大数与大数或大数与普通整数的差，可重载为运算符“-”
	Mul，乘，求大数与大数或大数与普通整数的积，可重载为运算符“*”
	Div，除，求大数与大数或大数与普通整数的商，可重载为运算符“/”
	Mod，模，求大数与大数或大数与普通整数的模，可重载为运算符“%”
	*****************************************************************/
	CBigInt& Mov(unsigned __int64 A);
	CBigInt& Mov(CBigInt& A);
	CBigInt Add(CBigInt& A);
	CBigInt Sub(CBigInt& A);
	CBigInt Mul(CBigInt& A);
	CBigInt Div(CBigInt& A);
	CBigInt Mod(CBigInt& A);
	CBigInt Add(unsigned long A);
	CBigInt Sub(unsigned long A);
	CBigInt Mul(unsigned long A);
	CBigInt Div(unsigned long A);
	unsigned long Mod(unsigned long A); 
	int Cmp(CBigInt& A);

	CBigInt& operator=(unsigned __int64 A){return Mov(A);}
	CBigInt& operator=(CBigInt& A){return Mov(A);}
	CBigInt operator+(CBigInt& A){return Add(A);}
	CBigInt operator-(CBigInt& A){return Sub(A);}
	CBigInt operator*(CBigInt& A){return Mul(A);}
	CBigInt operator/(CBigInt& A){return Div(A);}
	CBigInt operator%(CBigInt& A){return Mod(A);}
	CBigInt operator+(unsigned long A){return Add(A);}
	CBigInt operator-(unsigned long A){return Sub(A);}
	CBigInt operator*(unsigned long A){return Mul(A);}
	CBigInt operator/(unsigned long A){return Div(A);}
	unsigned long operator%(unsigned long A){return Mod(A);}
	bool operator==(CBigInt& A){return Cmp(A)==0;}
	bool operator!=(CBigInt& A){return Cmp(A)!=0;}
	bool operator>(CBigInt& A){return Cmp(A)>0;}
	bool operator>=(CBigInt& A){return Cmp(A)>=0;}
	bool operator<(CBigInt& A){return Cmp(A)<0;}
	bool operator<=(CBigInt& A){return Cmp(A)<=0;}
	/*****************************************************************
	输入输出
	FromString，从字符串按10进制或16进制格式输入到大数
	ToString，将大数按10进制或16进制格式输出到字符串
	*****************************************************************/
	void FromString(CAtlStringA& str, unsigned int system=HEX);
	void ToString(CAtlStringA& str, unsigned int system=HEX);
	
	CBigInt Euc(CBigInt& A);
	CBigInt RsaTrans(CBigInt& A, CBigInt& B);

	void SetByte(const UCHAR* buffer,int bufflen);
	const UCHAR* GetByte(int &bufflen);
};
class CPrimeCreator:public CBigInt
{
protected:
	IRandom* rand32;
public:
	CPrimeCreator(IRandom* rander=nullptr);
	~CPrimeCreator();
	/*****************************************************************
	RSA相关运算
	Rab，拉宾米勒算法进行素数测试
	Euc，欧几里德算法求解同余方程
	RsaTrans，反复平方算法进行幂模运算
	GetPrime，产生指定长度的随机大素数
	*****************************************************************/
	int Rab();
	int BaseTest();//基本质数表测试
	int EulerCriterion();//欧拉准则测试
	void GetPrime(int bits);
};

/*
//生成密钥
CBigInt P,Q,N,D,E;
int len=16;//1024bit密钥 512bit素数 占16个ULONG (2,4,8,16)
P.GetPrime(len);
Q.GetPrime(len);
N.Mov(P.Mul(Q));
P=P.Sub(1);
Q=Q.Sub(1);
P.Mov(P.Mul(Q));
D.Mov(0x10001);//理论推荐
E.Mov(D.Euc(P));
//使用密钥
char ppp[]="海盗王去低价位读卡器我恶趣味定期会晤俄乌德军诶打开哦恶起舞额";
CBigInt SS,SS2,SS3; //SS原文 SS2 密文 SS3解密
SS.SetByte((const UCHAR*)ppp,sizeof(ppp));
ATLASSERT(SS.Cmp(N)<0);
SS2.Mov(SS.RsaTrans(E,N));
SS3.Mov(SS2.RsaTrans(D,N));
int reslen;
const UCHAR* res=SS3.GetByte(reslen);
*/
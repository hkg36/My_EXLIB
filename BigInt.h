#pragma once
/*****************************************************************
���������ͷ�ļ���BigInt.h
���ߣ�afanty@vip.sina.com
�汾��1.2 (2003.5.13)
˵����������MFC��1024λRSA����
*****************************************************************/
#include <atlbase.h>
#include <atlstr.h>
#include "Random32.h"
class CBigInt
{
	enum{DEC=10,HEX=16};
	friend class CPrimeCreator;
protected:
	//��������1120λ�������ƣ����м���
	enum{BI_MAXLEN=35};
	//������0x100000000�����µĳ���    
	unsigned m_nLength;
	//�������¼������0x100000000������ÿһλ��ֵ
	unsigned long m_ulValue[BI_MAXLEN];
public:
	CBigInt();
	~CBigInt();

	/*****************************************************************
	��������������
	Mov����ֵ���㣬�ɸ�ֵΪ��������ͨ������������Ϊ�������=��
	Cmp���Ƚ����㣬������Ϊ�������==������!=������>=������<=����
	Add���ӣ��������������������ͨ�����ĺͣ�������Ϊ�������+��
	Sub�������������������������ͨ�����Ĳ������Ϊ�������-��
	Mul���ˣ��������������������ͨ�����Ļ���������Ϊ�������*��
	Div�������������������������ͨ�������̣�������Ϊ�������/��
	Mod��ģ���������������������ͨ������ģ��������Ϊ�������%��
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
	�������
	FromString�����ַ�����10���ƻ�16���Ƹ�ʽ���뵽����
	ToString����������10���ƻ�16���Ƹ�ʽ������ַ���
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
	RSA�������
	Rab�����������㷨������������
	Euc��ŷ������㷨���ͬ�෽��
	RsaTrans������ƽ���㷨������ģ����
	GetPrime������ָ�����ȵ����������
	*****************************************************************/
	int Rab();
	int BaseTest();//�������������
	int EulerCriterion();//ŷ��׼�����
	void GetPrime(int bits);
};

/*
//������Կ
CBigInt P,Q,N,D,E;
int len=16;//1024bit��Կ 512bit���� ռ16��ULONG (2,4,8,16)
P.GetPrime(len);
Q.GetPrime(len);
N.Mov(P.Mul(Q));
P=P.Sub(1);
Q=Q.Sub(1);
P.Mov(P.Mul(Q));
D.Mov(0x10001);//�����Ƽ�
E.Mov(D.Euc(P));
//ʹ����Կ
char ppp[]="������ȥ�ͼ�λ�������Ҷ�Ȥζ���ڻ�����ڵ¾�����Ŷ�������";
CBigInt SS,SS2,SS3; //SSԭ�� SS2 ���� SS3����
SS.SetByte((const UCHAR*)ppp,sizeof(ppp));
ATLASSERT(SS.Cmp(N)<0);
SS2.Mov(SS.RsaTrans(E,N));
SS3.Mov(SS2.RsaTrans(D,N));
int reslen;
const UCHAR* res=SS3.GetByte(reslen);
*/
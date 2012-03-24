#pragma once
#include <atlbase.h>
#include <atlstr.h>
#include <string>
/*
���ʽ������,������"((2-((35.46e-.1-12/7)*-2+56))*4/13)^3%13.6"���ַ���.
% ǿ����������
\ ǿ�����γ���
ExpressionΪ�������ֽ��
Expression2Ϊ��׺���ʽת����
*/
class Expression
{
private:
	double value;
	char op;
	Expression *Left;
	Expression *Right;
private:
	std::string DeleteParenthesis(const std::string &exp);  
	bool DivideExp      (const std::string &exp); 
	bool SeekAddSub     (const std::string & exp);    //��һ�����ʽ�ֽ��������,��һ���ְ�����һ�������򱻼���,�ڶ�����Ϊ������[+-],���µ���Ϊ��������.
	bool SeekMulDivMod  (const std::string & exp);    //��һ����ʽ�ֽ��������,��һ���ְ�����һ�������򱻳���,�ڶ�����Ϊ������[*/%],���µ���Ϊ��������.
	bool SeekExponent   (const std::string & exp);    //��һ�������ֽ��������,��һ���ְ�������,�ڶ�����Ϊȡָ������[^],���µ���Ϊ��������.

public:
	Expression(void);
	Expression(const std::string & exp);
	virtual ~Expression(void);
public:
	operator double(){return Result();}
	void SetExpression(const std::string & exp);
	double Result(void);
	double Result(const std::string & exp);
	static double Get(const std::string & exp);
};
namespace Expression2
{
	std::string trans(const char str[]) ;
	double compvalue(const char exp[]);
	double Get(const std::string & exp);
}


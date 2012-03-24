#pragma once
#include <atlbase.h>
#include <atlstr.h>
#include <string>
/*
表达式计算器,计算如"((2-((35.46e-.1-12/7)*-2+56))*4/13)^3%13.6"的字符串.
% 强制整形求余
\ 强制整形除法
Expression为二叉树分解版
Expression2为后缀表达式转换版
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
	bool SeekAddSub     (const std::string & exp);    //将一个表达式分解成三部分,第一部分包含第一个加数或被减数,第二部分为操作符[+-],余下的做为第三部分.
	bool SeekMulDivMod  (const std::string & exp);    //将一个因式分解成三部分,第一部分包含第一个因数或被除数,第二部分为操作符[*/%],余下的做为第三部分.
	bool SeekExponent   (const std::string & exp);    //将一个因数分解成三部分,第一部分包含底数,第二部分为取指操作符[^],余下的做为第三部分.

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


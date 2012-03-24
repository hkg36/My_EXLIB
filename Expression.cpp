#include "Expression.h"
#include <cmath>
#include <stdexcept>
#include <stack>
using namespace std;

Expression::Expression(void):op(0),value(0.0),Left(0),Right(0)
{
}

Expression::~Expression(void)
{
	if(Left)
	{
		delete Left;
		Left = 0;
	}
	if(Right)
	{
		delete Right;
		Right = 0;
	}
}

Expression::Expression(const std::string & exp):op(0),value(0.0),Left(0),Right(0)
{
	SetExpression(exp);
}

void Expression::SetExpression(const std::string & exp)
{
	//设置表达式串
	if(Left)
	{
		//确保左子树为空
		delete Left;
		Left = 0;
	}
	if(Right)
	{
		//确保右子树为空
		delete Right;
		Right = 0;
	}
	if(exp.empty())
	{
		//若是空串,则值为0,返回
		value = 0.0;
		return;
	}   
	if(!DivideExp(exp))    //检查是否可以被分解成两个部分,如果可分,则分解过程中不断产生新的子树
	{
		//若不可分,此时整个表达式有可能在括号里面,所以应该先去括号再取值.
		value = atof(DeleteParenthesis(exp).c_str());
	}
}

std::string Expression::DeleteParenthesis(const std::string & exp)
{
	//去括号:
	char ch=0;
	int id=0;
	int iParenthesisCount = 0;           //对括号进行计数,遇'('加1,遇')'减1
	if(exp[0]!='(') return exp;          //首字符不是左括号,则不需要去括号
	while(iParenthesisCount==0)
	{
		//该循环最多执行一次,目的是为了找到第一个左括号,如果是空串则将原空串返回
		ch=exp[id++];
		if(ch==0) return exp;            //直到最后也没有找到左括号,返回原表达式.
		if(ch=='(')iParenthesisCount++;  //一旦找到左括号,则计数加1,将导致循环结束
	}
	//既然已经到这儿了,说明肯定有左括号'(',并且括号计数值已不是0
	while(iParenthesisCount!=0)
	{
		ch=exp[id++];
		if(ch==0)                        //直到最后也没有找到匹配的括号,说明表达式中括号不匹配,抛出异常:
			throw invalid_argument("parentheses does not match!");
		if(ch=='(') iParenthesisCount++;
		if(ch==')') iParenthesisCount--;
	}
	if(exp[id]!=0)
		//找到了匹配的右括号,此时如果没有到结尾,说明表达式不全在一对括号内,这时不应该去括号,而应该返回原表达式串
		return exp;
	else
		//整个表达式全在一对括号内,此时应该去掉两端的括号,返回去掉两端括号之后的表达式串
		//感谢化石网友发现BUG：当输入表达式为((-3))时，返回的结果为0，这里应该递归调用DeleteParenthesis函数，确保去掉多余的括号。
		return DeleteParenthesis(exp.substr(1,exp.length()-2));
}

bool Expression::DivideExp(const std::string & exp)
{
	ATLASSERT(exp.find(' ')==std::string::npos
		&& exp.find('\r')==std::string::npos
		&& exp.find('\n')==std::string::npos
		&& exp.find('\t')==std::string::npos);
	//将一个表达式分解成两部分
	if(exp.empty())
	{
		//表达式为空时,将值置为0,返回真值的目的是为了告诉调用者不需要再对返回的结果进行处理了.
		value = 0.0;
		return true;
	}
	return SeekAddSub(DeleteParenthesis(exp));  //返回处理加法和减法的结果
}

bool Expression::SeekAddSub(const std::string & exp)
{
	int iParenthesisCount=0;                 //括号计数,遇'('增一,遇')'减一,从遇到'('之后,减到0时即找到了匹配的')'
	const char *cBuf=exp.c_str();     //复制表达式
	const char *pch =cBuf;                         //用于遍历表达式
	const char *pop=0;                             //指向找到的操作符[+-]
	while(*pch)
	{
		switch(*pch)
		{
		case '(':                            //遇到了'('
			iParenthesisCount++;
			while(iParenthesisCount>0 && (*pch))
			{
				pch++;                       //跳过括号:即直到找到匹配的')'
				if(*pch==')') --iParenthesisCount;
				else if(*pch=='(') ++iParenthesisCount;
			}
			break;
		case '+':
		case '-':
			if(pch==cBuf) break;
			if(*(pch-1)=='e' || *(pch-1)=='E') break;
			if(strchr("1234567890.)",*(pch-1))==nullptr)break;
			pop=pch;                         //总是指向最后一个,生成二叉树时,前面的都在左子树上,先计算
			break;
		}
		pch++;
	}
	if(pop)
	{
		//找到了加或减操作符,以操作符为界分为左右两部分
		std::string sl(cBuf,pop);
		std::string sr(pop+1,pch);
		op = *pop;                           //设置该节点的操作符
		Left = new Expression(sl);           //生成左子树
		Right = new Expression(sr);          //生成右子树
		return true;                         //返回真值,表示已分解为两部分
	}
	else
	{
		//没有找到加减运算符,再找乘除模运算符,并返回它的结果
		return SeekMulDivMod(exp);
	}
}

bool Expression::SeekMulDivMod(const std::string & exp)
{
	int iParenthesisCount=0;                 //括号计数,遇'('增一,遇')'减一,从遇到'('之后,减到0时即找到了匹配的')'
	const char *cBuf=exp.c_str(); 
	const char *pch =cBuf;                         //用于遍历表达式
	const char *pop=0;                             //指向找到的操作符[*/%]
	while(*pch)
	{
		switch(*pch)
		{
		case '(':
			//遇到了'('
			iParenthesisCount++;
			while(iParenthesisCount>0 && (*pch))
			{
				//跳过括号:即直到找到匹配的')'
				pch++;
				if(*pch==')') --iParenthesisCount;
				else if(*pch=='(') ++iParenthesisCount;
			}
			break;
		case '*':
		case '/':
		case '%':
		case '\\':
			pop=pch;//总是指向最后一个,生成二叉树时,前面的都在左子树上,先计算
			break;
		}
		pch++;
	}
	if(pop)
	{
		//找到了乘,除或模操作符,以操作符为界分为左右两部分
		std::string sl(cBuf,pop);
		std::string sr(pop+1,pch);
		op = *pop;                       //设置该节点的操作符
		Left = new Expression(sl);       //生成左子树
		Right = new Expression(sr);      //生成右子树
		return true;                     //返回真值,表示已分为两部分
	}
	else
	{
		//没有找到乘除模运算符,再长幂运算符,并返回处理结果
		return SeekExponent(exp);
	}
}

bool Expression::SeekExponent(const std::string & exp)
{
	int iParenthesisCount=0;                 //括号计数,遇'('增一,遇')'减一,从遇到'('之后,减到0时即找到了匹配的')'
	const char *cBuf=exp.c_str();
	const char *pch =cBuf;                         //用于遍历表达式
	const char *pop=0;                             //指向找到的操作符[^]
	while(*pch)
	{
		switch(*pch)
		{
		case '(':
			//遇到了'('
			iParenthesisCount++;
			while(iParenthesisCount>0 && (*pch))
			{
				//跳过括号:即直到找到匹配的')'
				pch++;
				if(*pch==')') --iParenthesisCount;
				else if(*pch=='(') ++iParenthesisCount;
			}
			break;
		case '^':
			pop=pch;//总是指向最后一个,生成二叉树时,前面的都在左子树上,先计算
			break;
		}
		pch++;
	}
	if(pop)
	{
		//找到了幂运算符,以操作符为界,将表达式分为两部分
		std::string sl(cBuf,pop);
		std::string sr(pop+1,pch);
		op = *pop;                       //设置该结点的操作符
		Left = new Expression(sl);       //生成左子树
		Right = new Expression(sr);      //生成右子树
		return true;                     //返回真值,表示已分成两部分
	}
	else
	{
		return false;                    //返回假值,表示不能再分了
	}
}

double Expression::Result(void)
{
	switch(op)
	{
	case '+':    value = Left->Result() + Right->Result();    break;
	case '-':    value = Left->Result() - Right->Result();    break;
	case '*':    value = Left->Result() * Right->Result();    break;
	case '/':    value = Left->Result() / Right->Result();    break;
	case '%':    value = int(Left->Result()) % int(Right->Result());    break;
	case '\\':   value = int(Left->Result() / Right->Result()); break;
	case '^':    value = pow(Left->Result(),Right->Result());   break;
	default:    break;
	}
	return value;
}

double Expression::Result(const std::string & exp)
{
	//直接计算给出的表达式的值,并返回
	SetExpression(exp);
	return Result();
}
double Expression::Get(const std::string & exp)
{
	std::string exp2;
	for(auto i=exp.begin();i!=exp.end();i++)
	{
		if(*i==' ' || *i=='\r' || *i=='\n' || *i=='\t') continue;
		exp2+=*i;
	}
	return Expression(exp2);
}

namespace Expression2
{
#define MaxSize 20
	inline bool isnumchar(const char c)
	{
		if(c>='0' && c<='9') return true;
		if(c=='e' ||  c=='E'  || c=='.') return true;
		return false;
	}
	struct data
	{
		const char a;
		const unsigned char level;
	};
	static const data priority[]=
	{
		'+',1,
		'-',1,
		'*',2,
		'/',2,
		'\\',2,
		'%',2,
		'^',3,
		'p',200,
	};
	inline bool isoperator(const char a)
	{
		for(size_t i=0;i<_countof(priority);i++)
		{
			if(priority[i].a==a)
			{
				return true;
			}
		}
		return false;
	}
	inline bool ispriority(const char a,const char b) // a>=b
	{
		unsigned char ap=0,bp=0;
		for(size_t i=0;i<_countof(priority);i++)
		{
			if(priority[i].a==a)
			{
				ap=priority[i].level;
				break;
			}
		}
		for(size_t i=0;i<_countof(priority);i++)
		{
			if(priority[i].a==b)
			{
				bp=priority[i].level;
				break;
			}
		}
		return ap>=bp;
	}
	std::string trans(const char str[])   /*将算术表达式转换成后追表达式*/
	{
		ATLASSERT(strchr(str,' ')==nullptr);
		std::string exp;     
		stack<char> op;                        
		char ch,lastch=0;
		ch=*str;                    /*将str的每一个数转换成ch*/
		str++;
		while(ch)               /*ch对应不同的符号的时候对应的转换情况*/
		{
			if(isnumchar(lastch)==false && lastch!=')' && ch=='-')
			{
				op.push('p');
				lastch=ch;
			}
			else if(ch=='(')
			{
				op.push(ch);
				lastch=ch;
			}
			else if(ch==')')
			{
				while(op.top()!='(')    
				{
					exp+=op.top();
					op.pop();
				}
				op.pop();
				lastch=ch;
			}
			else if(isoperator(ch))
			{
				while(!op.empty() && op.top()!='(' && ispriority(op.top(),ch))
				{
					exp+=op.top();
					op.pop();
				}
				op.push(ch);
				lastch=ch;
			}
			else
			{
				while(((lastch=='e' || lastch=='E') && ch=='-') || isnumchar(ch))
				{
					exp+=ch;
					lastch=ch;
					ch=*str;str++;
				}
				str--;
				exp+='#';
			}
			ch=*str;
			str++;
		}
		while(!op.empty())
		{
			exp+=op.top();
			op.pop();
		}
		return exp;
	}
	double compvalue(const char exp[])
	{
		stack<double> st;
		char ch;
		int t=0;
		ch=exp[t];
		t++;
		while(ch)
		{
			double tmp;
			switch(ch)
			{
			case'+':
				tmp=st.top();
				st.pop();
				st.top()=st.top()+tmp;
				break;
			case'-':
				tmp=st.top();
				st.pop();
				st.top()=st.top()-tmp;
				break;
			case'*':
				tmp=st.top();
				st.pop();
				st.top()=st.top()*tmp;
				break;
			case'/':
				tmp=st.top();
				st.pop();
				st.top()=st.top()/tmp;
				break;
			case '\\':
				tmp=st.top();
				st.pop();
				st.top()=(long)st.top()/(long)tmp;
				break;
			case '%':
				tmp=st.top();
				st.pop();
				st.top()=(long)st.top()%(long)tmp;
				break;
			case '^':
				tmp=st.top();
				st.pop();
				st.top()=pow(st.top(),tmp);
				break;
			case 'p':
				st.top()=-st.top();
				break;
			default:
				std::string numberbuf;
				while(ch!='#')
				{
					numberbuf+=ch;
					ch=exp[t];
					t++;
				}
				st.push(atof(numberbuf.c_str()));
			}
			ch=exp[t];
			t++;
		}
		return st.top();
	}
	double Get(const std::string & exp)
	{
		std::string exp2;
		for(auto i=exp.begin();i!=exp.end();i++)
		{
			if(*i==' ' || *i=='\r' || *i=='\n' || *i=='\t') continue;
			exp2+=*i;
		}
		return compvalue(exp2.c_str());
	}
}
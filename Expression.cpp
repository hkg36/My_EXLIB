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
	//���ñ��ʽ��
	if(Left)
	{
		//ȷ��������Ϊ��
		delete Left;
		Left = 0;
	}
	if(Right)
	{
		//ȷ��������Ϊ��
		delete Right;
		Right = 0;
	}
	if(exp.empty())
	{
		//���ǿմ�,��ֵΪ0,����
		value = 0.0;
		return;
	}   
	if(!DivideExp(exp))    //����Ƿ���Ա��ֽ����������,����ɷ�,��ֽ�����в��ϲ����µ�����
	{
		//�����ɷ�,��ʱ�������ʽ�п�������������,����Ӧ����ȥ������ȡֵ.
		value = atof(DeleteParenthesis(exp).c_str());
	}
}

std::string Expression::DeleteParenthesis(const std::string & exp)
{
	//ȥ����:
	char ch=0;
	int id=0;
	int iParenthesisCount = 0;           //�����Ž��м���,��'('��1,��')'��1
	if(exp[0]!='(') return exp;          //���ַ�����������,����Ҫȥ����
	while(iParenthesisCount==0)
	{
		//��ѭ�����ִ��һ��,Ŀ����Ϊ���ҵ���һ��������,����ǿմ���ԭ�մ�����
		ch=exp[id++];
		if(ch==0) return exp;            //ֱ�����Ҳû���ҵ�������,����ԭ���ʽ.
		if(ch=='(')iParenthesisCount++;  //һ���ҵ�������,�������1,������ѭ������
	}
	//��Ȼ�Ѿ��������,˵���϶���������'(',�������ż���ֵ�Ѳ���0
	while(iParenthesisCount!=0)
	{
		ch=exp[id++];
		if(ch==0)                        //ֱ�����Ҳû���ҵ�ƥ�������,˵�����ʽ�����Ų�ƥ��,�׳��쳣:
			throw invalid_argument("parentheses does not match!");
		if(ch=='(') iParenthesisCount++;
		if(ch==')') iParenthesisCount--;
	}
	if(exp[id]!=0)
		//�ҵ���ƥ���������,��ʱ���û�е���β,˵�����ʽ��ȫ��һ��������,��ʱ��Ӧ��ȥ����,��Ӧ�÷���ԭ���ʽ��
		return exp;
	else
		//�������ʽȫ��һ��������,��ʱӦ��ȥ�����˵�����,����ȥ����������֮��ı��ʽ��
		//��л��ʯ���ѷ���BUG����������ʽΪ((-3))ʱ�����صĽ��Ϊ0������Ӧ�õݹ����DeleteParenthesis������ȷ��ȥ����������š�
		return DeleteParenthesis(exp.substr(1,exp.length()-2));
}

bool Expression::DivideExp(const std::string & exp)
{
	ATLASSERT(exp.find(' ')==std::string::npos
		&& exp.find('\r')==std::string::npos
		&& exp.find('\n')==std::string::npos
		&& exp.find('\t')==std::string::npos);
	//��һ�����ʽ�ֽ��������
	if(exp.empty())
	{
		//���ʽΪ��ʱ,��ֵ��Ϊ0,������ֵ��Ŀ����Ϊ�˸��ߵ����߲���Ҫ�ٶԷ��صĽ�����д�����.
		value = 0.0;
		return true;
	}
	return SeekAddSub(DeleteParenthesis(exp));  //���ش���ӷ��ͼ����Ľ��
}

bool Expression::SeekAddSub(const std::string & exp)
{
	int iParenthesisCount=0;                 //���ż���,��'('��һ,��')'��һ,������'('֮��,����0ʱ���ҵ���ƥ���')'
	const char *cBuf=exp.c_str();     //���Ʊ��ʽ
	const char *pch =cBuf;                         //���ڱ������ʽ
	const char *pop=0;                             //ָ���ҵ��Ĳ�����[+-]
	while(*pch)
	{
		switch(*pch)
		{
		case '(':                            //������'('
			iParenthesisCount++;
			while(iParenthesisCount>0 && (*pch))
			{
				pch++;                       //��������:��ֱ���ҵ�ƥ���')'
				if(*pch==')') --iParenthesisCount;
				else if(*pch=='(') ++iParenthesisCount;
			}
			break;
		case '+':
		case '-':
			if(pch==cBuf) break;
			if(*(pch-1)=='e' || *(pch-1)=='E') break;
			if(strchr("1234567890.)",*(pch-1))==nullptr)break;
			pop=pch;                         //����ָ�����һ��,���ɶ�����ʱ,ǰ��Ķ�����������,�ȼ���
			break;
		}
		pch++;
	}
	if(pop)
	{
		//�ҵ��˼ӻ��������,�Բ�����Ϊ���Ϊ����������
		std::string sl(cBuf,pop);
		std::string sr(pop+1,pch);
		op = *pop;                           //���øýڵ�Ĳ�����
		Left = new Expression(sl);           //����������
		Right = new Expression(sr);          //����������
		return true;                         //������ֵ,��ʾ�ѷֽ�Ϊ������
	}
	else
	{
		//û���ҵ��Ӽ������,���ҳ˳�ģ�����,���������Ľ��
		return SeekMulDivMod(exp);
	}
}

bool Expression::SeekMulDivMod(const std::string & exp)
{
	int iParenthesisCount=0;                 //���ż���,��'('��һ,��')'��һ,������'('֮��,����0ʱ���ҵ���ƥ���')'
	const char *cBuf=exp.c_str(); 
	const char *pch =cBuf;                         //���ڱ������ʽ
	const char *pop=0;                             //ָ���ҵ��Ĳ�����[*/%]
	while(*pch)
	{
		switch(*pch)
		{
		case '(':
			//������'('
			iParenthesisCount++;
			while(iParenthesisCount>0 && (*pch))
			{
				//��������:��ֱ���ҵ�ƥ���')'
				pch++;
				if(*pch==')') --iParenthesisCount;
				else if(*pch=='(') ++iParenthesisCount;
			}
			break;
		case '*':
		case '/':
		case '%':
		case '\\':
			pop=pch;//����ָ�����һ��,���ɶ�����ʱ,ǰ��Ķ�����������,�ȼ���
			break;
		}
		pch++;
	}
	if(pop)
	{
		//�ҵ��˳�,����ģ������,�Բ�����Ϊ���Ϊ����������
		std::string sl(cBuf,pop);
		std::string sr(pop+1,pch);
		op = *pop;                       //���øýڵ�Ĳ�����
		Left = new Expression(sl);       //����������
		Right = new Expression(sr);      //����������
		return true;                     //������ֵ,��ʾ�ѷ�Ϊ������
	}
	else
	{
		//û���ҵ��˳�ģ�����,�ٳ��������,�����ش�����
		return SeekExponent(exp);
	}
}

bool Expression::SeekExponent(const std::string & exp)
{
	int iParenthesisCount=0;                 //���ż���,��'('��һ,��')'��һ,������'('֮��,����0ʱ���ҵ���ƥ���')'
	const char *cBuf=exp.c_str();
	const char *pch =cBuf;                         //���ڱ������ʽ
	const char *pop=0;                             //ָ���ҵ��Ĳ�����[^]
	while(*pch)
	{
		switch(*pch)
		{
		case '(':
			//������'('
			iParenthesisCount++;
			while(iParenthesisCount>0 && (*pch))
			{
				//��������:��ֱ���ҵ�ƥ���')'
				pch++;
				if(*pch==')') --iParenthesisCount;
				else if(*pch=='(') ++iParenthesisCount;
			}
			break;
		case '^':
			pop=pch;//����ָ�����һ��,���ɶ�����ʱ,ǰ��Ķ�����������,�ȼ���
			break;
		}
		pch++;
	}
	if(pop)
	{
		//�ҵ����������,�Բ�����Ϊ��,�����ʽ��Ϊ������
		std::string sl(cBuf,pop);
		std::string sr(pop+1,pch);
		op = *pop;                       //���øý��Ĳ�����
		Left = new Expression(sl);       //����������
		Right = new Expression(sr);      //����������
		return true;                     //������ֵ,��ʾ�ѷֳ�������
	}
	else
	{
		return false;                    //���ؼ�ֵ,��ʾ�����ٷ���
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
	//ֱ�Ӽ�������ı��ʽ��ֵ,������
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
	std::string trans(const char str[])   /*���������ʽת���ɺ�׷���ʽ*/
	{
		ATLASSERT(strchr(str,' ')==nullptr);
		std::string exp;     
		stack<char> op;                        
		char ch,lastch=0;
		ch=*str;                    /*��str��ÿһ����ת����ch*/
		str++;
		while(ch)               /*ch��Ӧ��ͬ�ķ��ŵ�ʱ���Ӧ��ת�����*/
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
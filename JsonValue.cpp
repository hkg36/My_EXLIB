#include "JsonLib.h"
namespace json2
{
	const ATL::CAtlStringW& CJsonValue::MatchExpectedToken(Token::Type nExpected,TokenStream& tokenStream)
	{
		if (tokenStream.EOS())
		{
			ATL::CAtlStringW sMessage = L"Unexpected End of token stream";
			throw sMessage; // nowhere to point to
		}

		const Token& token = tokenStream.Get();
		if (token.nType != nExpected)
		{
			ATL::CAtlStringW sMessage = L"Unexpected token: " + token.sValue;
			throw sMessage;
		}

		return token.sValue;
	}
	void CJsonValue::WriteString(const CAtlStringW s,OutputInterface &output)
	{
		int count=s.GetLength();
		for (int i=0; i<count; ++i)
		{
			wchar_t w=s[i];
			if(w<=128)
			{
				switch (w)
				{
				case '"':	output.write(&output,"\\\"",2);break;
				case '\\':	output.write(&output,"\\\\",2);break;
				case '\b':	output.write(&output,"\\b",2);break;
				case '\f':	output.write(&output,"\\f",2);break;
				case '\n':	output.write(&output,"\\n",2);break;
				case '\r':	output.write(&output,"\\r",2);break;
				case '\t':	output.write(&output,"\\t",2);break;
				default:	output.writeChar(&output,(char)w);break;
				}
			}
			else
			{
				const static wchar_t hex[]=L"0123456789ABCDEF";
				char hexres[4];
				for(int i=3;i>=0;i--)
				{
					hexres[i]=(char)hex[w%16];
					w/=16;
				}
				output.write(&output,"\\u",2);
				output.write(&output,hexres,4);
			}
		}
	}
	PJsonValue CJsonValue::GetJsonValueFromToken(Token::Type type)
	{
		switch (type) {
		case Token::TOKEN_OBJECT_BEGIN:
			{
				return new CJsonObject();
			}

		case Token::TOKEN_ARRAY_BEGIN:
			{
				return new CJsonArray();
			}

		case Token::TOKEN_STRING:
			{
				return new CJsonString();
			}

		case Token::TOKEN_NUMBER:
			{
				return new CJsonNumber();
			}

		case Token::TOKEN_BOOLEAN:
			{
				return new CJsonBool();
			}

		case Token::TOKEN_NULL:
			{
				return nullptr;
			}

		default:
			{
				ATL::CAtlStringW sMessage = L"Unexpected token.";
				throw sMessage;
			}
		}
		return nullptr;
	}
	PJsonValue CJsonValue::ParseBase(TokenStream& tokenStream) 
	{
		if (tokenStream.EOS()) {
			ATL::CAtlStringW sMessage = L"Unexpected end of token stream";
			throw sMessage; // nowhere to point to
		}
		const Token& token = tokenStream.Peek();
		PJsonValue newvalue=CJsonValue::GetJsonValueFromToken(token.nType);
		if(newvalue!=nullptr)
		{
			try
			{
				newvalue->Parse(tokenStream);
			}
			catch(CAtlStringW error)
			{
				throw error;
			}
		}
		return newvalue;
	}

	void CJsonString::Parse(TokenStream& tokenStream)
	{
		value = MatchExpectedToken(Token::TOKEN_STRING, tokenStream);
	}
	void CJsonString::Save(OutputInterface &output)
	{
		output.writeChar(&output,'"');
		CJsonValue::WriteString(value,output);
		output.writeChar(&output,L'"');
	}

	void CJsonNumber::Parse(TokenStream& tokenStream)
	{
		const ATL::CAtlStringW sValue = MatchExpectedToken(Token::TOKEN_NUMBER, tokenStream);

		double dValue;
		wchar_t c;
		// did we consume all characters in the token?
		if (swscanf_s(sValue,L"%lg%c",&dValue,&c)==2)
		{
			ATL::CAtlStringW sMessage = L"Unexpected character in NUMBER token: " + sValue;
			throw sMessage;
		}

		value = dValue;
	}
	void CJsonNumber::Save(OutputInterface &output)
	{
		CAtlStringA temp;
		temp.Format("%G",value);
		output.WriteCString(temp);
	}
	void CJsonBool::Parse(TokenStream& tokenStream)
	{
		const ATL::CAtlStringW sValue = MatchExpectedToken(Token::TOKEN_BOOLEAN, tokenStream);
		if(sValue.CompareNoCase(L"true")==0)
			value=true;
		else if(sValue.CompareNoCase(L"false")==0)
			value=false;
		else
		{
			ATL::CAtlStringW sMessage = L"Unexpected character in BOOLEAN token: " + sValue;
			throw sMessage;
		}
	}
	void CJsonBool::Save(OutputInterface &output)
	{
		if(value)
			output.write(&output,"true",4);
		else
			output.write(&output,"false",5);
	}

	void CJsonObject::PutValue(ATL::CAtlStringW name,PJsonValue toadd)
	{
		members.SetAt(name,toadd);
	}
	PJsonValue CJsonObject::Get(ATL::CAtlStringW name)
	{
		auto i=members.Lookup(name);
		if(i==nullptr)
			return nullptr;
		else
			return i->m_value;
	}
	CAtlStringW CJsonObject::GetString(ATL::CAtlStringW name)
	{
		PJsonString res=Get(name);
		if(res) return res->value;
		throw ATL::CAtlStringW( L"Type Error(String)");
	}
	double CJsonObject::GetNumber(ATL::CAtlStringW name)
	{
		PJsonNumber res=Get(name);
		if(res) return res->value;
		throw ATL::CAtlStringW(L"Type Error(Number)");
	}
	bool CJsonObject::GetBool(ATL::CAtlStringW name)
	{
		PJsonBool res=Get(name);
		if(res) return res->value;
		throw ATL::CAtlStringW(L"Type Error(Bool)");
	}
	void CJsonObject::Del(ATL::CAtlStringW name)
	{
		members.RemoveKey(name);
	}
	void CJsonObject::Clear()
	{
		members.RemoveAll();
	}
	CJsonObject::~CJsonObject()
	{
		Clear();
	}
	void CJsonObject::Parse(TokenStream& tokenStream)
	{
		MatchExpectedToken(Token::TOKEN_OBJECT_BEGIN, tokenStream);

		bool bContinue = (tokenStream.EOS() == false &&
			tokenStream.Peek().nType != Token::TOKEN_OBJECT_END);
		while (bContinue)
		{
			// first the member name. save the token in case we have to throw an exception
			const Token& tokenName = tokenStream.Peek();
			CAtlStringW name = MatchExpectedToken(Token::TOKEN_STRING, tokenStream);

			// ...then the key/value separator...
			MatchExpectedToken(Token::TOKEN_MEMBER_ASSIGN, tokenStream);

			// ...then the value itself (can be anything).
			//Parse(uele, tokenStream);
			PJsonValue res=CJsonValue::ParseBase(tokenStream);
			// try adding it to the object (this could throw)
			if(res==nullptr)
				MatchExpectedToken(Token::TOKEN_NULL, tokenStream);
			this->PutValue(name,res);

			bContinue = (tokenStream.EOS() == false &&
				tokenStream.Peek().nType == Token::TOKEN_NEXT_ELEMENT);
			if (bContinue)
				MatchExpectedToken(Token::TOKEN_NEXT_ELEMENT, tokenStream);
		}

		MatchExpectedToken(Token::TOKEN_OBJECT_END, tokenStream);
	}
	void CJsonObject::Save(OutputInterface &output)
	{
		if (members.IsEmpty())
			output.write(&output,"{}",2);
		else
		{
			output.writeChar(&output,L'{');

			POSITION pos=members.GetHeadPosition();
			while (true) {
				auto it=members.GetNext(pos);
				output.writeChar(&output,L'"');
				CJsonValue::WriteString(it->m_key,output);
				output.write(&output,"\":",2);
				if(it->m_value)
					it->m_value->Save(output);
				else
					output.write(&output,"null",4);

				if (pos!=nullptr)
					output.writeChar(&output,L',');
				else break;
			}

			output.writeChar(&output,'}');
		}
	}

	void CJsonArray::Clear()
	{
		members.RemoveAll();
	}
	CJsonArray::~CJsonArray()
	{
		Clear();
	}
	void CJsonArray::PutValue(PJsonValue value)
	{
		members.Add(value);
	}
	PJsonValue CJsonArray::Get(size_t index)
	{
		if(members.GetCount()>index)
			return members[index];
		else return nullptr;
	}
	CAtlStringW CJsonArray::GetString(size_t index)
	{
		PJsonString res=Get(index);
		if(res) return res->value;
		throw ATL::CAtlStringW(L"Type Error(String)");
	}
	double CJsonArray::GetNumber(size_t index)
	{
		PJsonNumber res=Get(index);
		if(res) return res->value;
		throw ATL::CAtlStringW(L"Type Error(Number)");
	}
	bool CJsonArray::GetBool(size_t index)
	{
		PJsonBool res=Get(index);
		if(res) return res->value;
		throw ATL::CAtlStringW(L"Type Error(Bool)");
	}

	void CJsonArray::Del(size_t index)
	{
		if(members.GetCount()>index)
		{
			members.RemoveAt(index);
		}
	}
	void CJsonArray::Parse(TokenStream& tokenStream)
	{
		MatchExpectedToken(Token::TOKEN_ARRAY_BEGIN, tokenStream);

		bool bContinue = (tokenStream.EOS() == false &&
			tokenStream.Peek().nType != Token::TOKEN_ARRAY_END);
		while (bContinue)
		{
			PJsonValue res=CJsonValue::ParseBase(tokenStream);
			if(res==nullptr) MatchExpectedToken(Token::TOKEN_NULL, tokenStream);
			members.Add(res);

			bContinue = (tokenStream.EOS() == false &&
				tokenStream.Peek().nType == Token::TOKEN_NEXT_ELEMENT);
			if (bContinue)
				MatchExpectedToken(Token::TOKEN_NEXT_ELEMENT, tokenStream);
		}

		MatchExpectedToken(Token::TOKEN_ARRAY_END, tokenStream);
	}
	void CJsonArray::Save(OutputInterface &output)
	{
		if (members.IsEmpty())
			output.write(&output,"[]",2);
		else
		{
			output.writeChar(&output,'[');

			size_t i=0;
			size_t count=members.GetCount();
			while (true) {
				PJsonValue value=members.GetAt(i);
				if(value)
					value->Save(output);
				else
					output.write(&output,"null",4);
				i++;
				if (i<members.GetCount())
					output.writeChar(&output,',');
				else break;
			}

			output.writeChar(&output,']');
		}
	}

	struct ParseStackEle
	{
		PJsonValue now;
		CAtlStringW name;
	};
	/*
	PJsonValue CJsonValue::ParseBase2(TokenStream& tokenStream)
	{
		PJsonValue root=nullptr;
		stack<ParseStackEle> praseStack;
		ParseStackEle newone;
		while(tokenStream.EOS()==false)
		{
			const Token &token=tokenStream.Get();
			switch(token.nType)
			{
			case Token::TOKEN_OBJECT_BEGIN:  //    {
				{
					newone.now=new CJsonObject();
					newone.name.Empty();
					praseStack.push(newone);
					if(root==nullptr)
						root=newone.now;
				}
				break;
			case Token::TOKEN_OBJECT_END:    //    }
				{
					if(praseStack.empty() || praseStack.top().now->GetJsonValueType()!=JsonValueType::Object)
						throw CAtlStringW(L"Parse Fail Object");
					PJsonValue lastTop=praseStack.top().now;
					praseStack.pop();
					if(praseStack.empty()==false)
					{
						ParseStackEle &nowone=praseStack.top();
						PJsonObject jobj=nowone.now;
						if(jobj)
						{
							if(nowone.name.IsEmpty())
								nowone.name=token.sValue;
							else
							{
								jobj->Put(nowone.name,lastTop);
							}
							break;
						}
						PJsonArray jarray=nowone.now;
						if(jarray)
						{
							jarray->Put(lastTop);
						}
					}
				}
				break;
			case Token::TOKEN_ARRAY_BEGIN:   //    [
				{
					newone.now=new CJsonArray();
					newone.name.Empty();
					praseStack.push(newone);
					if(root==nullptr)
						root=newone.now;
				}
				break;
			case Token::TOKEN_ARRAY_END:    //    ]
				{
					if(praseStack.top().now->GetJsonValueType()!=JsonValueType::Array)
						throw CAtlStringW(L"Parse Fail Array");
					PJsonValue lastTop=praseStack.top().now;
					praseStack.pop();
					if(praseStack.empty()==false)
					{
						ParseStackEle &nowone=praseStack.top();
						PJsonObject jobj=nowone.now;
						if(jobj)
						{
							if(nowone.name.IsEmpty())
								nowone.name=token.sValue;
							else
							{
								jobj->Put(nowone.name,lastTop);
							}
							break;
						}
						PJsonArray jarray=nowone.now;
						if(jarray)
						{
							jarray->Put(lastTop);
						}
					}
				}
				break;
			case Token::TOKEN_NEXT_ELEMENT:  //    ,
				if(praseStack.top().now->GetJsonValueType()==JsonValueType::Object)
					praseStack.top().name.Empty();
				break;
			case Token::TOKEN_MEMBER_ASSIGN: //    :
				{
					if(praseStack.empty())
						throw CAtlStringW("TOKEN_MEMBER_ASSIGN error");
					ParseStackEle &nowone=praseStack.top();
					if( !(nowone.now->GetJsonValueType()==JsonValueType::Object && nowone.name.IsEmpty()==FALSE))
						throw CAtlStringW("TOKEN_MEMBER_ASSIGN error2");
				}
				break;
			case Token::TOKEN_STRING:        //    "xxx"
				if(praseStack.empty())
					root=new CJsonString(token.sValue);
				else
				{
					ParseStackEle &nowone=praseStack.top();
					PJsonObject jobj=nowone.now;
					if(jobj)
					{
						if(nowone.name.IsEmpty())
							nowone.name=token.sValue;
						else
						{
							jobj->Put(nowone.name,token.sValue);
						}
						break;
					}
					PJsonArray jarray=nowone.now;
					if(jarray)
					{
						jarray->Put(token.sValue);
					}
				}
				break;
			case Token::TOKEN_NUMBER:        //    [+/-]000.000[e[+/-]000]
				{
					double dValue;
					wchar_t c;
					if (swscanf_s(token.sValue,L"%lg%c",&dValue,&c)==2)
					{
						ATL::CAtlStringW sMessage = L"Unexpected character in NUMBER token: " + token.sValue;
						throw sMessage;
					}

					if(praseStack.empty())
						root=new CJsonNumber(dValue);
					else
					{
						ParseStackEle &nowone=praseStack.top();
						PJsonObject jobj=nowone.now;
						if(jobj)
						{
							if(nowone.name.IsEmpty())
								throw CAtlStringW("parse Fail");
							else
							{
								
								jobj->Put(nowone.name,dValue);
							}
							break;
						}
						PJsonArray jarray=nowone.now;
						if(jarray)
						{
							jarray->Put(dValue);
						}
					}
				}
				break;
			case Token::TOKEN_BOOLEAN:       //    true -or- false
				{
					bool bValue=false;
					if(token.sValue.CompareNoCase(L"true")==0)
						bValue=true;
					else if(token.sValue.CompareNoCase(L"false")==0)
						bValue=false;
					else
					{
						ATL::CAtlStringW sMessage = L"Unexpected character in BOOLEAN token: " + token.sValue;
						throw sMessage;
					}

					if(praseStack.empty())
						root=new CJsonBool(bValue);
					else
					{
						ParseStackEle &nowone=praseStack.top();
						PJsonObject jobj=nowone.now;
						if(jobj)
						{
							if(nowone.name.IsEmpty())
								throw CAtlStringW("parse Fail");
							else
							{
								jobj->Put(nowone.name,bValue);
							}
							break;
						}
						PJsonArray jarray=nowone.now;
						if(jarray)
						{
							jarray->Put(bValue);
						}
					}
				}break;
			case Token::TOKEN_NULL:           //    null
				{
					ParseStackEle &nowone=praseStack.top();
					PJsonObject jobj=nowone.now;
					if(jobj)
					{
						if(nowone.name.IsEmpty())
							throw CAtlStringW("parse Fail");
						else
						{
							jobj->Put(nowone.name,PJsonValue());
						}
						break;
					}
					PJsonArray jarray=nowone.now;
					if(jarray)
					{
						jarray->Put(PJsonValue());
					}
				}
				break;
			}
		}
		return root;
	}*/
}
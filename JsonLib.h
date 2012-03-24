#pragma once
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>
#include "cexarray.h"
#include "OutputInterface.h"
namespace json2
{
	struct Token
	{
		enum Type
		{
			TOKEN_OBJECT_BEGIN,  //    {
			TOKEN_OBJECT_END,    //    }
			TOKEN_ARRAY_BEGIN,   //    [
			TOKEN_ARRAY_END,     //    ]
			TOKEN_NEXT_ELEMENT,  //    ,
			TOKEN_MEMBER_ASSIGN, //    :
			TOKEN_STRING,        //    "xxx"
			TOKEN_NUMBER,        //    [+/-]000.000[e[+/-]000]
			TOKEN_BOOLEAN,       //    true -or- false
			TOKEN_NULL           //    null
		};
		Type nType;
		ATL::CAtlStringW sValue;
	};
	typedef std::deque<Token> Tokens;
	class TokenStream
	{
	public:
		inline TokenStream(const Tokens& tokens) :
		m_Tokens(tokens),
			m_itCurrent(tokens.begin())
		{}

		inline const Token& Peek() const {
			ATLASSERT(m_itCurrent != m_Tokens.end());
			return *(m_itCurrent); 
		}

		inline const Token& Get() {
			ATLASSERT(m_itCurrent != m_Tokens.end());
			return *(m_itCurrent++); 
		}

		inline bool EOS() const {
			return m_itCurrent == m_Tokens.end(); 
		}
	private:
		const Tokens& m_Tokens;
		Tokens::const_iterator m_itCurrent;
	};
	struct JsonValueType
	{
		enum
		{
			String=1,
			Number,
			Bool,
			Object,
			Array
		};
	};
	
	class CJsonValue;
	struct JsonValueIPtrCast
	{
		template<class OtherType>
		inline const CJsonValue* Cast(const OtherType* far_p) const
		{
			far_p->GetJsonValueType();
			return far_p;
		}
	};
	typedef CIPtr<CJsonValue,JsonValueIPtrCast> PJsonValue;
	class CJsonValue:public IPtrBase<CJsonValue,long>
	{
	private:
		int type;
	public:
		inline int GetJsonValueType()const{return type;}
		CJsonValue(int type):type(type){}
		virtual ~CJsonValue(void){}
		virtual void Parse(TokenStream& tokenStream)=0;
		virtual void Save(OutputInterface &output){};
	protected:
		inline const ATL::CAtlStringW& MatchExpectedToken(Token::Type nExpected,TokenStream& tokenStream);
		inline static void WriteString(const CAtlStringW s,OutputInterface &output);
		static PJsonValue GetJsonValueFromToken(Token::Type type);
	public:
		static PJsonValue ParseBase(TokenStream& tokenStream);
		//static PJsonValue ParseBase2(TokenStream& tokenStream);
	};


	class CJsonString:public CJsonValue
	{
	public:
		const static int mytype=JsonValueType::String;
		CAtlStringW value;
		CJsonString(CAtlStringW value):CJsonValue(mytype),value(value){}
		CJsonString():CJsonValue(mytype){};
		void Parse(TokenStream& tokenStream);
		void Save(OutputInterface &output);
	};
	class CJsonNumber:public CJsonValue
	{
	public:
		const static int mytype=JsonValueType::Number;
		double value;
		CJsonNumber():CJsonValue(mytype){}
		CJsonNumber(double value):CJsonValue(mytype),value(value){}
		void Parse(TokenStream& tokenStream);
		void Save(OutputInterface &output);
	};
	class CJsonBool:public CJsonValue
	{
	public:
		const static int mytype=JsonValueType::Bool;
		bool value;
		CJsonBool():CJsonValue(mytype){}
		CJsonBool(bool value):CJsonValue(mytype),value(value){}
		void Parse(TokenStream& tokenStream);
		void Save(OutputInterface &output);
	};
	class CJsonObject:public CJsonValue
	{
	private:
		typedef CRBMap<ATL::CAtlStringW,PJsonValue> Members;
		Members members;
	public:
		const static int mytype=JsonValueType::Object;
		void PutValue(ATL::CAtlStringW name,PJsonValue toadd=nullptr);
		void Put(ATL::CAtlStringW name,ATL::CAtlStringW v)
		{
			CIPtr<CJsonString> newjso=new CJsonString();
			newjso->value=v;
			PutValue(name,newjso);
		}
		void Put(ATL::CAtlStringW name,double v)
		{
			CIPtr<CJsonNumber> newjso=new CJsonNumber();
			newjso->value=v;
			PutValue(name,newjso);
		}

		void PutBool(ATL::CAtlStringW name,bool v)
		{
			CIPtr<CJsonBool> newjso=new CJsonBool();
			newjso->value=v;
			PutValue(name,newjso);
		}
		PJsonValue Get(ATL::CAtlStringW name);
		CAtlStringW GetString(ATL::CAtlStringW name);
		double GetNumber(ATL::CAtlStringW name);
		bool GetBool(ATL::CAtlStringW name);
		void Del(ATL::CAtlStringW name);
		void Clear();
		CJsonObject():CJsonValue(mytype){}
		~CJsonObject();
		void Parse(TokenStream& tokenStream);
		void Save(OutputInterface &output);
	};
	class CJsonArray:public CJsonValue
	{
	private:
		typedef CAtlArray<PJsonValue> Members;
		Members members;
	public:
		const static int mytype=JsonValueType::Array;
		void Clear();
		CJsonArray():CJsonValue(mytype){}
		~CJsonArray();
		inline size_t Size(){return members.GetCount();}
		void PutValue(PJsonValue value);
		void Put(ATL::CAtlStringW v)
		{
			CIPtr<CJsonString> newjso=new CJsonString();
			newjso->value=v;
			PutValue(newjso);
		}
		void Put(double v)
		{
			CIPtr<CJsonNumber> newjso=new CJsonNumber();
			newjso->value=v;
			PutValue(newjso);
		}
		void PutBool(bool v)
		{
			CIPtr<CJsonBool> newjso=new CJsonBool();
			newjso->value=v;
			PutValue(newjso);
		}
		PJsonValue Get(size_t index);
		CAtlStringW GetString(size_t index);
		double GetNumber(size_t index);
		bool GetBool(size_t index);

		void Del(size_t index);
		void Parse(TokenStream& tokenStream);
		void Save(OutputInterface &output);
	};

	template<class TYPE>
	struct JsonIPtrCast
	{
		template<class OtherType>
		inline const TYPE* Cast(const OtherType* far_p) const
		{
			if(TYPE::mytype==far_p->GetJsonValueType())
			{
				return static_cast<const TYPE*>(far_p);
			}
			else
				return nullptr;
		}
	};
	typedef CIPtr<CJsonString,JsonIPtrCast<CJsonString> > PJsonString;
	typedef CIPtr<CJsonNumber,JsonIPtrCast<CJsonNumber>> PJsonNumber;
	typedef CIPtr<CJsonBool,JsonIPtrCast<CJsonBool>> PJsonBool;
	typedef CIPtr<CJsonObject,JsonIPtrCast<CJsonObject>> PJsonObject;
	typedef CIPtr<CJsonArray,JsonIPtrCast<CJsonArray>> PJsonArray;

	class CJsonReader
	{
	public:
		static PJsonValue Prase(LPCSTR istr,unsigned int strlen);
		static PJsonValue Prase(InputInterface& inputstream);
	private:
		static void Scan(Tokens& tokens, InputInterface& inputStream);
		inline static char EatWhiteSpace(InputInterface& inputStream);
		static void MatchExpectedString(const ATL::CAtlStringW& sExpected, InputInterface& inputStream);
		static void MatchString(ATL::CAtlStringW& string, InputInterface& inputStream);
		static void MatchNumber(ATL::CAtlStringW& sNumber,InputInterface& inputStream);
	};
}
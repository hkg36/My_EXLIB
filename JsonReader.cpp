#include "JsonLib.h"
#include <exception>
namespace json2
{
	inline char CJsonReader::EatWhiteSpace(InputInterface& inputStream)
	{
		while(true)
		{
			char res=inputStream.Peek();
			if(res==0) return 0;
			if(::isspace(res)) inputStream.Get();
			else return res;
		}
	}
	PJsonValue CJsonReader::Prase(LPCSTR istr,unsigned int strlen)
	{
		InputStream inputstream(istr,strlen);
		return Prase(inputstream);
	}
	PJsonValue CJsonReader::Prase(InputInterface& inputstream)
	{
		Tokens tokens;
		Scan(tokens,inputstream);

		TokenStream tokenStream(tokens);
		return CJsonValue::ParseBase(tokenStream);
	}
	void CJsonReader::Scan(Tokens& tokens, InputInterface& inputStream)
	{
		while (EatWhiteSpace(inputStream))
		{
			// if all goes well, we'll create a token each pass
			Token token;

			// gives us null-terminated string
			wchar_t checkchar=inputStream.Peek();

			switch (checkchar)
			{
			case '{':
				inputStream.Get();
				token.nType = Token::TOKEN_OBJECT_BEGIN;
				break;

			case '}':
				inputStream.Get();
				token.nType = Token::TOKEN_OBJECT_END;
				break;

			case '[':
				inputStream.Get();
				token.nType = Token::TOKEN_ARRAY_BEGIN;
				break;

			case ']':
				inputStream.Get();
				token.nType = Token::TOKEN_ARRAY_END;
				break;

			case ',':
				inputStream.Get();
				token.nType = Token::TOKEN_NEXT_ELEMENT;
				break;

			case ':':
				inputStream.Get();
				token.nType = Token::TOKEN_MEMBER_ASSIGN;
				break;

			case '"':
				MatchString(token.sValue, inputStream);
				token.nType = Token::TOKEN_STRING;
				break;

			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				MatchNumber(token.sValue, inputStream);
				token.nType = Token::TOKEN_NUMBER;
				break;

			case 't':
				token.sValue = L"true";
				MatchExpectedString(token.sValue, inputStream);
				token.nType = Token::TOKEN_BOOLEAN;
				break;

			case 'f':
				token.sValue = L"false";
				MatchExpectedString(token.sValue, inputStream);
				token.nType = Token::TOKEN_BOOLEAN;
				break;

			case 'n':
				token.sValue = L"null";
				MatchExpectedString(token.sValue, inputStream);
				token.nType = Token::TOKEN_NULL;
				break;

			default: {
				ATL::CAtlStringW sErrorMessage = L"Unexpected character in stream: ";
				sErrorMessage.AppendChar(checkchar);
				throw sErrorMessage;
					 }
			}

			tokens.push_back(token);
		}
	}

	void CJsonReader::MatchExpectedString(const ATL::CAtlStringW& sExpected, InputInterface& inputStream)
	{
		int count=sExpected.GetLength();
		for (int i=0 ; i<count; i++) {
			char res=inputStream.Get();
			if (res==0 || res != sExpected[i]) // ...or did we find something different?
			{
				ATL::CAtlStringW sMessage = L"Expected string: " + sExpected;
				throw sMessage;
			}
		}

		// all's well if we made it here, return quietly
	}

	void CJsonReader::MatchString(ATL::CAtlStringW& string, InputInterface& inputStream)
	{
		MatchExpectedString(L"\"", inputStream);

		while (true)
		{
			wchar_t c = inputStream.Get();
			if(c==0 || c=='\"') break;
			// escape?
			if (c == '\\')
			{
				c=inputStream.Get();
				switch (c)
				{
				case '"':      string.AppendChar('"');     break;
				case '\\':     string.AppendChar('\\');    break;
				case 'b':      string.AppendChar('\b');    break;
				case 'f':      string.AppendChar('\f');    break;
				case 'n':      string.AppendChar('\n');    break;
				case 'r':      string.AppendChar('\r');    break;
				case 't':      string.AppendChar('\t');    break;
				case 'u': 
					{
						wchar_t ichar=0;
						for(int i=0;i<4;i++)
						{
							wchar_t res=inputStream.Get();
							switch(res)
							{
							case 'a':
							case 'A': ichar = ichar*16+10; break;
							case 'b':
							case 'B': ichar = ichar*16+11; break;
							case 'c':
							case 'C': ichar = ichar*16+12; break;
							case 'd':
							case 'D': ichar = ichar*16+13; break;
							case 'e':
							case 'E': ichar = ichar*16+14; break;
							case 'f':
							case 'F': ichar = ichar*16+15; break;
							case '0':
							case '1':
							case '2':
							case '3':
							case '4':
							case '5':
							case '6':
							case '7':
							case '8':
							case '9': ichar = ichar*16+(res - '0'); break;
							default:
								{
									ATL::CAtlStringW sMessage = L"Unexpected char in decode unicode: \\";
									sMessage.AppendChar(res);
									throw sMessage;
								}break;
							}
						}
						string.AppendChar(ichar);
					}break;
				default: {
					ATL::CAtlStringW sMessage = L"Unrecognized escape sequence found in string: \\";
					sMessage.AppendChar(c);
					throw sMessage;
						 }
				}
			}
			else {
				string.AppendChar(c);
			}
		}

		// eat the last '"' that we just peeked
		//MatchExpectedString(L"\"", inputStream);
	}


	void CJsonReader::MatchNumber(ATL::CAtlStringW& sNumber, InputInterface& inputStream)
	{
		const wchar_t sNumericChars[]={'+','-','.','0','1','2','3','4','5','6','7','8','9','E','e',};

		while (true)
		{
			wchar_t c=inputStream.Peek();
			if(c==0) return;
			const wchar_t* res=std::lower_bound(sNumericChars,sNumericChars+_countof(sNumericChars),c);
			if(*res!=c || res==(sNumericChars+_countof(sNumericChars)))return;
			sNumber.AppendChar(inputStream.Get());   
		}
	}
}

#include "lexer.h"
#include "easylogging++.h"

namespace par{
/* token operators*/

	bool Token::operator==(const TokenType& _oth)
	{
		return _oth == type;
	}

	bool Token::operator==(const Token& _oth)
	{
		return _oth.type == type && _oth.begin == begin && _oth.end == end;
	}

	bool Token::operator!=(const TokenType _oth)
	{
		return _oth != type;
	}

	bool Token::operator!=(const Token& _oth)
	{
		return  _oth.begin != begin || _oth.end != end;
	}

	/* lexer */

Lexer::Lexer(std::string& _text)
	: m_text(_text)
{
}


Lexer::~Lexer()
{
}

void Lexer::analyse()
{
	//reset
	m_tokens.resize(0);

	unsigned int begin = 0;
	unsigned int end = 0;

	//state for word / number reading
	int lookEnd = 0;

	//'.' count in numbers
	int count;
	bool isHex = false;

	size_t size = m_text.size();

	for (size_t i = 0; i < size; ++i)
	{
		if (!lookEnd)
		{
			//probably a comment
			if (m_text[i] == '/')
			{
				//jump over the comment segment
				//a line
				if (m_text[i + 1] == '/')
				{
					i = m_text.find('\n', i + 1);
					//no line break appears when this is the last line
					if (i == std::string::npos) i = size;
					//continue evaluating the segment after the comment
					continue;
				}
				//until comment is closed by "*/"
				// + 1 because the index of the first char is returned by find
				else if ((m_text[i + 1] == '*'))
				{
					i = m_text.find("*/", i + 2) + 1;
					//continue evaluating the segment after the comment
					continue;
				}
				//division operator
				else if (m_text[i + 1] == '=')
				{
					begin = i;
					i++;
					m_tokens.emplace_back(TokenType::Operator, begin, i);
				}
				else m_tokens.emplace_back(TokenType::Operator, i, i);
			}

			//const string
			else if (m_text[i] == '"')
			{
				lookEnd = 3;
				begin = i;
			}

			//numeral begins
			else if (m_text[i] >= '0' && m_text[i] <= '9')
			{
				lookEnd = 2;

				//'.' count
				count = 0;
				begin = i;

				//hex number
				if (m_text[i] == '0' && m_text[i + 1] == 'x')
				{
					isHex = true;
					//jump over the prefix "0x"
					i += 2;
				}
			}

			//expression end
			else if (m_text[i] == ';') m_tokens.emplace_back(TokenType::End, i, i);

			//curly brackets
			else if (m_text[i] == '{') m_tokens.emplace_back(TokenType::CurlyBracketLeft, i, i);
			else if (m_text[i] == '}') m_tokens.emplace_back(TokenType::CurlyBracketRight, i, i);

			//parenthesis
			else if (m_text[i] == '(') m_tokens.emplace_back(TokenType::ParenthesisLeft, i, i);
			else if (m_text[i] == ')') m_tokens.emplace_back(TokenType::ParenthesisRight, i, i);

			//squarebracket
			else if (m_text[i] == '[') m_tokens.emplace_back(TokenType::SquareBracketLeft, i, i);
			else if (m_text[i] == ']') m_tokens.emplace_back(TokenType::SquareBracketRight, i, i);

			//comma
			else if (m_text[i] == ',') m_tokens.emplace_back(TokenType::Comma, i, i);


			//file structuring chars are skipped
			else if (m_text[i] == ' ' || m_text[i] == '\n' || m_text[i] == '\r' || m_text[i] == '\t')
			{
			}

			//string starts with a letter
			else if ((m_text[i] >= 0x41 && m_text[i] <= 0x5A) || (m_text[i] >= 0x61 && m_text[i] <= 0x7A) || isValidChar(m_text[i]))
			{
				lookEnd = 1;
				//save index of the first char to return it later on
				begin = i;
			}

			//anything else is an operator
			else
			{
				begin = i;
				//some operators consist of two chars
				if (m_text[i + 1] == m_text[i] || m_text[i + 1] == '=')
					i++;

				m_tokens.emplace_back(TokenType::Operator, begin, i);
			}
		}
		//lookEnd = 1 means that the end of a word is searched
		//any char that cannot be part of a symbol terminates the word
		else if (lookEnd == 1)
		{
			if (!(m_text[i] >= '0' && m_text[i] <= '9') && !(m_text[i] >= 0x41 && m_text[i] <= 0x5A) && !(m_text[i] >= 0x61 && m_text[i] <= 0x7A) && !isValidChar(m_text[i]))
			{
				//decrement the index counter so that it points to the last char of the word
				//causes revaluation of the termination char in the next iteration as it could be ';' or an operator
				i--;
				m_tokens.emplace_back(TokenType::Symbol, begin, i);

				//convert to upper case
				for (int j = begin; j <= i; ++j)
					m_text[j] = ::tolower(m_text[j]);

				//return to default state
				lookEnd = 0;
			}
		}
		//numeral
		// no numchar or a second '.' terminate the numeral
		else if (lookEnd == 2 )
		{
			if (m_text[i] == '.') count++;
			if (!(m_text[i] >= '0' && m_text[i] <= '9') && ((m_text[i] != '.') || (count > 1)))
			{
				//hex numbers allow A-F as letters
				if (isHex && m_text[i] >= 'A' && m_text[i] <= 'F') continue;

				i--;

				//check previous tokens as it might be a minus sign
				if (i >= 0 && m_text[begin - 1] == '-')
				{
					std::list < Token >::iterator it = m_tokens.end();
					--it;
					//check whether it is a operator in this case
					if (it->type != TokenType::Constant && it->type != TokenType::Symbol)
					{
						//add the sign to the token
						begin--;
						m_tokens.pop_back();
					}
				}
				//contains a '.' -> float
				m_tokens.emplace_back(count ? TokenType::ConstFloat : TokenType::ConstInt, begin, i);

				lookEnd = 0;
			}
		}
		//const string
		//the '"' are discarded
		else if (lookEnd == 3 && m_text[i] == '"')
		{
			m_tokens.emplace_back(TokenType::ConstStr, begin + 1, i - 1);
			lookEnd = 0;
		}
	}

	m_iterator = m_tokens.begin();
}

Token* Lexer::nextToken()
{
	return (m_iterator != m_tokens.end() ? &*m_iterator++ : nullptr);
}

std::string Lexer::getWord(Token& _token)
{
	return m_text.substr(_token.begin, _token.end + 1 - _token.begin);
}

int Lexer::getInt(Token& _token)
{
	//since this is a crucial operation string allocation is to be prevented
	/*int val = 0;
	for (int i = _token.end; i <= _token.begin; ++i)
		val = val * 10 + (m_text[i] - '0');// last numeral is has a value of one*/
//	bool isHex = m_text[_token.begin] == '0' && m_text[_token.begin] == 'x';
//	int begin = isHex ? _token.begin + 2 : _token.begin;
	
	//strtol(&m_text[begin], nullptr, isHex ? 16 : 10);
	return strtol(&m_text[_token.begin], nullptr, 0);//atoi(&m_text[_token.begin]);
}

float Lexer::getFloat(Token& _token)
{
	return strtof(&m_text[_token.begin], nullptr);
}

bool Lexer::compare(const Token& _token, const std::string& _str)
{
	return !m_text.compare(_token.begin, 1 + _token.end - _token.begin, _str);
}

//additional characters that are valid in symbol names
const std::string validChars = "_^@üöäÜÄÖ";

bool Lexer::isValidChar(char _c)
{
	return !(validChars.find(_c) == std::string::npos);
}

}//end namespace
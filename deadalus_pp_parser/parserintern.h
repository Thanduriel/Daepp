#pragma once

#include "symbolext.h"
#include "parexception.h"

//data structures with wich the parser works
// that do not appear outside


//macros
#define PARSINGERROR(a,b) { throw ParsingError(a, b, m_lexer); }
//{parserLog(Error, a, b); return -1;} 

//checks a Token* and handles the error
#define NULLCHECK(a) if(!a) {throw ParsingError("Unexepected end of file.");} 

//easy token reading from the stream including error checks
#define TOKEN(type) {Token* tokenIntern = m_lexer->nextToken(); NULLCHECK(tokenIntern); if(*tokenIntern != type) PARSINGERROR("Unexpected token.",tokenIntern) }
//declares a token pointer with the given name
//thus it is not in its own scope
#define TOKENEXT(type, tokenPtr) Token* tokenPtr = m_lexer->nextToken(); NULLCHECK(tokenPtr); if(*tokenPtr != type) PARSINGERROR("Unexpected token.",tokenPtr)
#define TOKENOPT(type) (((tokenOpt = m_lexer->nextToken()) && *tokenOpt == type))

//the standard check for ';' that takes into account the setting "alwaysSemikolon"
#define SEMIKOLON if (!TOKENOPT(End)){if (m_config.m_alwaysSemicolon){PARSINGERROR("Expression end';' expected.", tokenOpt);}else m_lexer->prev();}

/* Array *************
 * A simple and light dynamic buffer with size = capacity
 * and only one initialisation
 */
template< typename _T >
class Array
{
public:
	Array(size_t _size) : m_size(_size) 
	{
		m_data = new _T[_size];
	};

	Array() : m_size(0)
	{
	};

	~Array()
	{
		if(m_size) delete[] m_data;
	}

	//direct access, does no boundry checks
	_T& operator[](size_t _i)
	{
		return m_data[_i];
	};

	//does not copy the old data
	//should only be used if not inizialised yet
	void resize(size_t _size)
	{
		m_data = new _T[_size];
		m_size = _size;
	}

	inline size_t size()
	{
		return m_size;
	}
private:
	size_t m_size;
	_T*	m_data;
};

namespace par{

	typedef game::Symbol_Type Namespace;

	/*a token wich can hold terms
	 * operands(terms are valid aswell) are managed in a tree structure
	 */
	class MathSymbol : public game::DummyOperator
	{
	public:
		/* MathSymbol() *********************
		 * @param _val value/index of the operator
		 * @param _size amount of params the operator has
		 * @param _token the related token
		 */
		MathSymbol(int _val, size_t _size, Token& _token) : param(_size), token(_token) { value = _val; isOperator = true; };
		MathSymbol(int _val, Token& _token) : token(_token) { value = _val; isOperator = true; };

		Array< game::Symbol_Core* > param; //params or operands

		game::ByteCodeStack stackInstruction;

		Token& token; // the token, that represents this operator in the source code
	};

	/* a token that substitutes array var while holding the correct index*/
	class ArraySymbol : public game::Symbol
	{
	public:
		ArraySymbol(game::Symbol& _sym, size_t _index):
			game::Symbol(_sym),
			index(_index),
			symbol(&_sym)
		{}

		game::Symbol* symbol;
		size_t index;
	};

	// a symbol whichs declaration has not been parsed yet
	class UndeclaredSymbol : public game::Symbol
	{
	public:
		UndeclaredSymbol(const std::string& _name, Token& _token) :
			Symbol(_name, false),
			token(_token)
		{}

		//move constructor
		//required to allow std::vector::erase
		//does only copying since an UndeclaredSymbol does not possess any dynamic data beside its name
		UndeclaredSymbol(UndeclaredSymbol&& _other): 
			Symbol(_other),
			token(_other.token)
		{ }

		UndeclaredSymbol& operator=(UndeclaredSymbol&& other) 
		{ 
			name = std::move(other.name);
			token = other.token;
			return *this; 
		}

		Token& token; //keep reference for error msgs
	};

	/*
	 * A code block that is yet to be parse.
	 */
	class CodeToParse
	{
	public:
		CodeToParse(TokenIterator _tokenIt, game::Symbol_Function& _function, Namespace* _namespace = nullptr) :
			m_tokenIt(_tokenIt),
			m_function(_function),
			m_namespace(_namespace)
		{
		};
	
		TokenIterator m_tokenIt; //token referencing to the '{'
		game::Symbol_Function& m_function; //function that takes the code
		Namespace* m_namespace;
	};
}
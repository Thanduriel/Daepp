#pragma once

#include "symboltable.h"
#include "symbol.h"
#include "lexer.h"

//data structures with wich the parser works
// that do not appear outside


//macros
#define PARSINGERROR(a,b) {parserLog(Error, a, b); return -1;} 

//checks a Token* and handles the error
#define NULLCHECK(a) if(!a) {m_lexer.prev(); parserLog(Error, "Unexepected end of file.", m_lexer.nextToken()); return -1;} 

//easy token reading from the stream including error checks
#define TOKEN(type) {Token* tokenIntern = m_lexer.nextToken(); NULLCHECK(tokenIntern); if(*tokenIntern != type) PARSINGERROR("Unexpected token.",tokenIntern) }
//declares a token pointer with the given name
//thus it is not in its own scope
#define TOKENEXT(type, tokenPtr) Token* tokenPtr = m_lexer.nextToken(); NULLCHECK(tokenPtr); if(*tokenPtr != type) PARSINGERROR("Unexpected token.",tokenPtr)
#define TOKENOPT(type) (((tokenOpt = m_lexer.nextToken()) && *tokenOpt == type))


/* Array *************
 * A simple dynamic buffer with size = capacity
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

		std::vector < game::StackInstruction > stackInstruction;

		Token& token; // the token, that represents this operator in the source code
	};

}
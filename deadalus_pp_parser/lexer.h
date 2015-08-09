#pragma once
#include <list>
#include <string>

namespace par{

	//types that the lexer can differentiate

	const int OPERATORTOKEN = 42;

enum TokenType
{
	Symbol, // charset of gothic strings: A-Z, a-z, _, @, ^
	Constant,
	ConstInt,
	ConstFloat,
	ConstStr,
	Operator,
	End,
	ParenthesisLeft,
	ParenthesisRight,
	CurlyBracketLeft,
	CurlyBracketRight,
	SquareBracketLeft,
	SquareBracketRight,
	Comma,
	Point,
	OpInc = OPERATORTOKEN, //start at some arbitary place so that conversion to Operator is simple translation
	OpDec,
	OpAdd,
	OpSub,
	OpMul,
	OpDiv,
	OpMod,
	OpOr,
	OpAnd,
	OpLess,
	OpHigher,
	OpLShift,
	OpRShift,
	OpLEq,
	OpHEq,
	OpEq,
	OpAssignment,
	OpAssignmentMul,
	OpAssignmentDiv,
	OpAssignmentAdd,
	OpAssignmentSub,
	OpEqual,
	OpNotEqual

};

struct Token
{
	//default constructor
	//only used to fullfill template conditions
	Token(){};

	Token(TokenType _type, unsigned int _begin, unsigned int _end)
		: type(_type),
		begin(_begin),
		end(_end)
	{};
	
	TokenType type;
	unsigned int begin;
	unsigned int end;

	//comparison operators
	bool operator==(const TokenType& _oth);
	bool operator==(const Token& _oth);

	bool operator>=(const TokenType& _oth);
	bool operator>=(const Token& _oth);

	bool operator!=(const TokenType _oth);
	bool operator!=(const Token& _oth);
};


class Lexer
{
public:
	Lexer(std::string& _text);
	~Lexer();

	/* analyse() ***************************
	 * Analysis the given text and creates tokens from it
	 * preserving the order
	 * resets the token stream
	 */
	void analyse();

	/* access operators*/

	/* nextToken() ************************
	 * @return next token in the stream or nullptr if the end is reached 
	 */
	Token* nextToken();

	/* prev() ******************************
	 * decrements the stream to the previous token
	 */
	void prev() { m_iterator--; };

	/* getWord() ***************************
	 * Retrieves the token as string
	 */
	std::string getWord(Token& _token);

	/* getInt() ***************************
	* Retrieves the token as a integer
	*/
	int getInt(Token& _token);

	/* getFloat() ***************************
	* Retrieves the token as a float
	*/
	float getFloat(Token& _token);

	/* compare() ***************************
	* compares a token with a string
	* this function is faster then using getWord() and "=="
	* @return 1 when equal
	*/
	bool compare(const Token& _token, const std::string& _str);

private:
	std::list < Token > m_tokens;
	std::list < Token >::iterator m_iterator;

	std::string& m_text;
};

}
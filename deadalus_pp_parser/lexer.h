#pragma once
#include <list>
#include <vector>
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


typedef std::vector < Token > TokenContainer;
typedef TokenContainer::iterator TokenIterator;

class Lexer
{
public:
	Lexer() = default;
	Lexer(std::string& _dataName);
	~Lexer();

	/* analyse() ***************************
	 * Analysis the given text and creates tokens from it
	 * preserving the order
	 * resets the token stream
	 */
	void analyse(std::string&& _text);

	/* access operators*/

	/* nextToken() ************************
	 * @return next token in the stream or nullptr if the end is reached 
	 */
	Token* nextToken();

	/* prev() ******************************
	 * decrements the stream to the previous token
	 */
	void prev() { m_iterator--; };

	Token* peek() { auto tok = nextToken(); prev(); return tok; };

	/* tokenToIterator() *********************
	 * Only looks left of the current token, asuming that you did not jump back
	 * @return the iterator to the given token
	 */
	TokenIterator tokenToIterator(par::Token& _tok) { auto newIt = m_iterator; while (_tok != *newIt) newIt--; return newIt; };

	/* setTokenIt() *************************
	 * sets the stream to the given iterator
	 * thus nextToken() will be the one after this iterator
	 */
	void setTokenIt(TokenIterator& _it) { m_iterator = _it; };

	/* setItToToken()
	 * Sets the iterator to point to the current check.
	 * Throws an error when the token is not in the list.
	 */
	void setItToToken(par::Token& _tok) { m_iterator = m_tokens.begin(); while (_tok != *m_iterator) m_iterator++; };

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

	const std::string& getRawText() { return m_text; };
	void setDataName(const std::string& _str) { m_dataName = _str; };
	const std::string& getDataName() { return m_dataName; };
private:
	inline bool isValidChar(char _c);
	//std::list
	//performance with list: 0.4547
	TokenContainer m_tokens;
	TokenIterator m_iterator;

	std::string m_text;
	std::string m_dataName; // a name associated with the text
};

}
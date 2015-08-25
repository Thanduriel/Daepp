#pragma once

#include <string>
#include "lexer.h"
#include "compiler.h"
#include "parserintern.h"

namespace par {

	enum LogLvl
	{
		Error = 16,
		Warning = 32
	};

/* Parser *******************************
 * The main part of the pipline
 * analyzes the content
 * error codes:
 * -1 parsing error
 * 0 success
 */

class Parser
{
public:
	Parser(const std::string& _configFile);
	~Parser();

	/* parseSource() ************************************
	 * parses a .src file with any files named within
	 */
	void parseSource(const std::string& _fileName);

	/* parseFile() ************************************
	* parses a .d file containing code
	*/
	int parseFile(const std::string& _fileName);


	/* compile() *************************************
	 * Compiles the parsed data into a (hopefully) by gothic readable .dat
	 */
	void compile() { m_compiler.compile("test.dat"); };

private:
	/* Term() *********************************
	* reads a mathematical term from the tokenstream
	* @param _value the calculated value of the term
	* can hold any int or float
	* @return parsing error code
	*/
	int Term(game::Symbol_Core* _ret = nullptr, game::Symbol_Function* _function = nullptr);

	int pushInstr(game::Symbol_Core* _sym, std::vector< game::StackInstruction >& _instrStack);

	inline int pushParamInstr(game::Symbol_Core* _sym, std::vector< game::StackInstruction >& _instrStack);

	//pushes the code that assigns the values on the stack to the function params
	void assignFunctionParam(game::Symbol_Function& _func);

	/* verifyParam() *****************************
	* @return true when the types match
	* takes into account implicit typecast and consts
	*/
	bool verifyParam(game::Symbol& _expected, game::Symbol_Core& _found);

	/* parseInstruction() ***********************
	* reads code from the tokenstream and translates it to bytecode
	* @param _token first token of the instruction
	* @param _stack the stack where the bytecode is pushed to
	*/
	int parseCodeBlock(par::Token& _token, game::Symbol_Function& _functionSymbol);
	int parseCodeBlock(game::Symbol_Function& _functionSymbol) { return parseCodeBlock(*m_lexer.nextToken(), _functionSymbol); };

	/* declareVar() *********************************
	 * reads the type and adds the symbol to the table
	 * @param _const declares a constant whichs value cannot be changed after init
	 * @param _table the SymbolTable the symbol should be added to
	 * @return parsing error code
	 */
	int declareVar(bool _const, game::SymbolTable< game::Symbol >& _table);

	/* declareFunc() ********************************
	 */
	int declareFunc();

	/* declareClass() *********************************
	* reads the type and adds the symbol to the table
	*/
	int declareClass();

	/* declareInstance() ********************************
	*/
	int declareInstance();

	/* declarePrototype *****************************
	*/
	int declarePrototype();

	/* parserLog() **********************************
	 * A wrapper for LOG(lvl) that
	 * prints the filename aswell as the line number
	 */
	void parserLog(LogLvl _lvl, const std::string& _msg, Token* _token = nullptr);

	inline int getType(Token& _token);

	//tokenizer
	Lexer m_lexer;

	//
	Compiler m_compiler;

	//config
	std::string m_sourceDir;
	bool m_caseSensitive;
	bool m_alwaysSemikolon;

	std::string m_currentFileName; //< name of the parsed file
	std::string m_currentFile; //< content of the currently parsed file
	//size_t m_pp; //<parser pointer
	int m_lineCount; //< current line


	game::Symbol_Type* m_currentNamespace;
	game::SymbolTable < UndeclaredSymbol > m_undeclaredSymbols;
	//resolved content
	game::GameData m_gameData;
};

}
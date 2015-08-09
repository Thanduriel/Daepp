#pragma once
#include "gamedata.h"


namespace par{

	/* Compiler *****************************
	 * compiles gameData into bytecode
	 */

	class Compiler
	{
	public:
		Compiler(game::GameData& _gameData);

		/* compile() ************************
		 * Compiles the given gameData into bytecode written in a file
		 * @param _outputFile the file the code is written to
		 *
		 */
		int compile(const std::string& _outputFile);

	private:
		int compileSortedTable(std::ofstream& _stream);

		//writes a single given symbol to the stream
		int compileSymbol(std::ofstream& _stream, game::Symbol& _sym);

		int compileClass(std::ofstream& _stream, game::Symbol_Type& _sym);

		//countes the size on the stack and adds the code offsets to the function symbol
		int compileFunction(std::ofstream& _stream, game::Symbol_Function& _sym);
		
		int compileStack(std::ofstream& _stream);

		game::GameData& m_gameData;

		int stackSize; //holds the current size of the stack in byte

		unsigned int parent; // parent of the currently written symbol
	};

}


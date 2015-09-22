#include "symbolext.h"
#include <deque>//currently based on this datastructure
//todo test performance with: vector, deque, list?

namespace game{


	struct GameData
	{
		GameData(std::initializer_list< Symbol_Type >& _init) : m_types(_init)
		{};

		game::SymbolTable< game::Symbol > m_symbols;

		//const symbols are resolved before compilation
		game::SymbolTable< game::ConstSymbol<int, 2> > m_constInts;
		game::SymbolTable< game::ConstSymbol_String > m_constStrings; //game::ConstSymbol<std::string, 3>
		game::SymbolTable< game::ConstSymbol<float, 1> > m_constFloats;

		game::SymbolTable< game::Symbol_Type > m_types;

		game::SymbolTable< game::Symbol_Function > m_functions;

		game::SymbolTable< game::Symbol_Instance > m_instances;
		game::SymbolTable< game::Symbol_Instance > m_prototypes;

		//not a SymbolTable since their names are generated and thus not an identifier
		std::deque< game::ConstSymbol_String > m_internStrings;
	};
}
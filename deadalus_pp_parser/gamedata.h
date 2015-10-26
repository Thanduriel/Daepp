#include "symbolext.h"
#include "refcontainer.h"
#include <deque>//currently based on this datastructure
//#include "fusedcontainer.h"
#include <memory>
//todo test performance with: vector, deque, list?

#define COMPONENTCONTAINER(a) boo::ComponentContainer<  a , game::Symbol > 

namespace game{

	struct GameData
	{
		GameData(std::initializer_list< game::Symbol_Type* >& _init) : m_types(_init)
		{
			g_atomTypeCount = m_types.size();
		};

		//all symbols with unique name
		utils::SymbolTable < game::Symbol > m_symbols;

		//references for faster lookup
		//todo test performance vs just the SymbolTable

		//const symbols are resolved before compilation
		//should be of type game::ConstSymbol<,>
		utils::ReferenceContainer < game::Symbol > m_constSymbols;
		
		//types including the basic types and classes
		utils::ReferenceContainer < game::Symbol_Type > m_types;
		utils::ReferenceContainer < game::Symbol_Instance > m_prototypes;

		//not part of all symbols since their names are generated and thus not an identifier
		//and no searching for them takes place
		std::deque< game::ConstSymbol_String > m_internStrings;

	};
}
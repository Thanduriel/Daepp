#include "symbolext.h"
#include "refcontainer.h"
#include <deque>//currently based on this datastructure
//#include "fusedcontainer.h"
#include <memory>
//todo test performance with: vector, deque, list?

#define COMPONENTCONTAINER(a) boo::ComponentContainer<  a , game::Symbol > 

namespace game{

	/* GameData
	 * A structure that can hold any symbol declarations
	 * and provides space for faster lookup wihle parsing.
	 * See compiler.h or parser.h for specific format usage.
	 */
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

		//functions (only real functions) to fix params and locals
		utils::ReferenceContainer < game::Symbol_Function > m_functions;

		//function variables -> need an id update
		utils::ReferenceContainer < game::ConstSymbol_Func > m_constVarFuncs;

		//Const strings are not part of m_symbols since their names are generated and thus not an identifier
		//and no searching for them takes place.
		//However their references need to remain valid.
		std::deque< std::unique_ptr < game::Symbol> > m_internStrings;

	};
}
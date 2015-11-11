#include "symbolext.h"

size_t g_atomTypeCount = 0;

namespace game{
	//int Symbol::idCount = 0;

	const std::array< game::Instruction, referenceParamInstructionCount > referenceParamInstructions =
	{
		game::Instruction::assign,
		game::Instruction::assignStr,
		Instruction::assignInst,
		game::Instruction::callExtern,//call is translated to a direct stack adr just below
		game::Instruction::setInst,
		game::Instruction::pushVar,
		game::Instruction::pushInst,
		game::Instruction::pushArray
	};
}
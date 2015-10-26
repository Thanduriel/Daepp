#include "reservedsymbols.h"

namespace lang{

	Operator::Operator(const std::string& _op, int _precedence, Associativity _associativity, int _paramCount, std::initializer_list< std::pair < const unsigned int, OperatorFunction > > _init)
		:op(_op),
		precedence(_precedence),
		associativity(_associativity),
		overloads(_init),
		paramCount(_paramCount)
	{
	};


	Operator::Operator(const std::string& _op, int _precedence, Associativity _associativity, int _paramCount, game::Instruction _stdInstr, std::initializer_list< std::pair < const unsigned int, OperatorFunction > > _init)
		:op(_op),
		precedence(_precedence),
		associativity(_associativity),
		overloads(_init),
		paramCount(_paramCount)
	{
		overloads.emplace(0x00020002, [=](OPPARAMLIST)
		{
			_stack->emplace_back(_stdInstr);

			return new game::Symbol_Core(2);
		});

		overloads.emplace(0x00020008, [=](OPPARAMLIST)
		{
	/*		_stack.emplace_back(game::Instruction::pushVar, _operand0->id);
			auto* operand1 = (game::ConstSymbol < int, 8 >*)(_operand1);
			_stack.emplace_back(game::Instruction::pushInt, operand1->value[0]);*/
			_stack->emplace_back(_stdInstr);

			return new game::Symbol_Core(2);
		});
	};


	game::Symbol_Core* Operator::toByteCode(game::Symbol_Core* _operand0, game::Symbol_Core* _operand1, game::ByteCodeStack* _stack)
	{
		auto it = overloads.find(generateKey(_operand0, _operand1));

		if (it == overloads.end())
		{
			it = overloads.find(generateKey(_operand1, _operand0));
			if (it != overloads.end())
			{
				std::swap(_operand0, _operand1);
			}
			else return nullptr;
		}

		return (*it).second(_operand0, _operand1, _stack);
	}

	// **************************************************** //

	unsigned int Operator::generateKey(game::Symbol_Core* _operand0, game::Symbol_Core* _operand1)
	{
		return (_operand1->type > 10 ? 7 : _operand1->type) | ((_operand0->type > 10 ? 7 : _operand0->type) << 16);
	}
}
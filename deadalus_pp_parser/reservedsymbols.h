#pragma once

#include <string>
#include <array>
#include <functional>
#include <unordered_map>

#include "symbolext.h"

#define MAKEKEY(a,b) (a | (b << 16))

//operator param list
#define OPPARAMLIST game::Symbol_Core* _operand0, game::Symbol_Core* _operand1, game::ByteCodeStack* _stack

//generic const float and int operations
// with a beeing the operation in c++
#define CONSTOPERATION(a) { 0x00080008, [](OPPARAMLIST){auto* operand0 = (game::DummyInt*)(_operand0);auto* operand1 = (game::DummyInt*)(_operand1);return new game::DummyInt(operand0->value a operand1->value);} },{ 0x00080009, [](OPPARAMLIST){auto* operand0 = (game::DummyInt*)(_operand0);auto* operand1 = (game::DummyFloat*)(_operand1);return new game::DummyFloat((float)operand0->value a operand1->value);} },{ 0x00090009, [](OPPARAMLIST){auto* operand0 = (game::DummyFloat*)(_operand0);auto* operand1 = (game::DummyFloat*)(_operand1);return new game::DummyFloat(operand0->value a operand1->value);} }

namespace lang{

	enum Associativity
	{
		LtR,
		RtL
	};

	typedef std::function< game::Symbol_Core* (game::Symbol_Core*, game::Symbol_Core*, game::ByteCodeStack*)> OperatorFunction;


	struct Operator
	{
		Operator(const std::string& _op, int _precedence, Associativity _associativity, int _paramCount,
			std::initializer_list< std::pair < const unsigned int , OperatorFunction > > _init = {});

		/* 
		 * @param _stdInstr an bytecode instruction that is aquivalent to the parsed operator in integer operations
		 * this includes (SymbolInt, SymbolInt); (SymbolInt, ConstInt); (ConstInt, ConstInt)
		 */
		Operator(const std::string& _op, int _precedence, Associativity _associativity, int _paramCount, game::Instruction _stdInstr,
			std::initializer_list< std::pair < const unsigned int, OperatorFunction > > _init = {});

		std::string op; //string that represents this operator in the code
		int precedence;
		Associativity associativity;

		int paramCount; //amount of params the operator has

		// @return a dummy symbol that contains the resulting type of the operation and, when inlined the value
		game::Symbol_Core* toByteCode(game::Symbol_Core* _operand0, game::Symbol_Core* _operand1, game::ByteCodeStack* _stack = nullptr);
	
	private:
		inline unsigned int generateKey(game::Symbol_Core* _operand0, game::Symbol_Core* _operand1);

		std::unordered_map < unsigned int, OperatorFunction > overloads;

	};

	const unsigned int operatorCount = 27;

	static std::array<Operator, operatorCount> operators =
	{
		Operator("++", 2, LtR, 1),
		Operator("--", 2, LtR, 1),
		Operator(".", 2, LtR, 2, 
		{
			{ 0x00070002, [](OPPARAMLIST)
			{
				return new game::Symbol_Core(2);
			} },
			{ 0x00070003, [](OPPARAMLIST)
			{
				return new game::Symbol_Core(3);
			} }
		}),
		Operator("+", 6, LtR, 2,game::Instruction::Add,
		{
			{ 0x00080008, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyInt*)(_operand0);
				auto* operand1 = (game::DummyInt*)(_operand1);

				return new game::DummyInt( operand0->value + operand1->value );
			} },
			{ 0x00080009, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyInt*)(_operand0);
				auto* operand1 = (game::DummyFloat*)(_operand1);

				return new game::DummyFloat((float)operand0->value + operand1->value);
			} },
			{ 0x00090009, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyFloat*)(_operand0);
				auto* operand1 = (game::DummyFloat*)(_operand1);

				return new game::DummyFloat(operand0->value + operand1->value);
			} }
		}),
		Operator("-", 6, LtR, 2, game::Instruction::Sub,
		{
				CONSTOPERATION(-)
		}),
		Operator("*", 5, LtR, 2, game::Instruction::Mul,
		{
				CONSTOPERATION(*)
		}),
		Operator("/", 5, LtR, 2, game::Instruction::Div,
		{
				CONSTOPERATION(/)
		}),
		Operator("%", 5, LtR, 2),
		Operator("<<", 7, LtR, 2, game::Instruction::shl,
		{
			{ 0x00080008, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyInt*)(_operand0);
				auto* operand1 = (game::DummyInt*)(_operand1);

				return new game::DummyInt(operand0->value << operand1->value);
			} }
		}),
		Operator(">>", 7, LtR, 2, game::Instruction::shr,
		{
			{ 0x00080008, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyInt*)(_operand0);
				auto* operand1 = (game::DummyInt*)(_operand1);

				return new game::DummyInt(operand0->value >> operand1->value);
			} }
		}),
		Operator("<", 8, LtR, 2, game::Instruction::Less,
		{
			CONSTOPERATION(<)
		}),
		Operator(">", 8, LtR, 2, game::Instruction::Higher,
		{
			CONSTOPERATION(>)
		}),
		Operator("<=", 8, LtR, 2,game::Instruction::leq),
		Operator(">=", 8, LtR, 2,game::Instruction::heg),
		Operator("==", 9, LtR, 2, game::Instruction::eq,
			{
				{ 0x00070007, [](OPPARAMLIST)
				{
					_stack->emplace_back(game::Instruction::eq);

					return new game::Symbol_Core(7);
				} },
				{ 0x00020007, [](OPPARAMLIST)
				{
					_stack->emplace_back(game::Instruction::eq);

					return new game::Symbol_Core(7);
				} }
			}),
		Operator("!=", 9, LtR, 2, game::Instruction::neq),
		Operator("&", 10, LtR, 2),
		Operator("^", 11, LtR, 2),
		Operator("|", 12, LtR, 2, game::Instruction::Or,
		{
			{ 0x00080008, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyInt*)(_operand0);
				auto* operand1 = (game::DummyInt*)(_operand1);

				return new game::DummyInt(operand0->value | operand1->value);
			} }
		}),
		Operator("&&", 13, LtR, 2, game::Instruction::land),
		Operator("||", 14, LtR, 2, game::Instruction::lor),
		Operator("=", 15, RtL, 2,
		{
			{ 0x00070007, [](OPPARAMLIST)
			{
				_stack->emplace_back(game::Instruction::assignInst);

				return new game::Symbol_Core(7);
			} },
			{ 0x00020002, [](OPPARAMLIST)
			{
				_stack->emplace_back(game::Instruction::assign);

				return new game::Symbol_Core(2);
			} },
			{ 0x00020008, [](OPPARAMLIST)
			{
				_stack->emplace_back(game::Instruction::assign);

				return new game::Symbol_Core(2);
			} },
			{ 0x00010009, [](OPPARAMLIST)
			{
				_stack->emplace_back(game::Instruction::assignFloat);

				return new game::Symbol_Core(1);
			} },
			{ 0x00030003, [](OPPARAMLIST)
			{
				_stack->emplace_back(game::Instruction::assignStr);

				return new game::Symbol_Core(3);
			} },
			{ 0x00050005, [](OPPARAMLIST)
			{
				_stack->emplace_back(game::Instruction::assignFunc);

				return new game::Symbol_Core(5);
			} }
		}),
		Operator("+=", 15, RtL, 2, game::Instruction::assignAdd),
		Operator("-=", 15, RtL, 2, game::Instruction::assignSub),
		Operator("*=", 15, RtL, 2, game::Instruction::assignMul),
		Operator("/=", 15, RtL, 2, game::Instruction::assignDiv),
		Operator("u", 3, LtR, 1, game::Instruction::uMinus,
		{
			{ 0x00080008, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyInt*)(_operand0);

				return new game::DummyInt(-operand0->value);
			} },
			{ 0x00090009, [](OPPARAMLIST)
			{
				auto* operand0 = (game::DummyFloat*)(_operand0);

				return new game::DummyFloat(-operand0->value);
			} }
		})
	};
}
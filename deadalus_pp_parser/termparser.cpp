#include "easylogging++.h"

#include "parser.h"
#include "reservedsymbols.h"

using namespace std;

namespace par{


	int Parser::Term(game::Symbol_Core* _ret, game::Symbol_Function* _function)
	{
		std::vector< game::StackInstruction >* instrStack = _function ? &_function->byteCode : nullptr;

		//Shunting-yard algorithm
		//http://en.wikipedia.org/wiki/Shunting-yard_algorithm

		//inlined symbols
		std::vector < std::unique_ptr < game::Symbol_Core > > dummySymbols;
		//somehow destruction goes wrong when a Symbol is added to dummySymbols
		std::vector < std::unique_ptr < game::Symbol > > dummyArraySymbols;

		//functions that are stored on the stack
		//transformation to a stack token: (index + 11) * -1
		std::vector < game::Symbol_Function* > functionSymbols;

		//value que
		std::list< game::Symbol_Core* > outputQue;
		//operator stack
		std::vector< MathSymbol* > operatorStack;

		Token* token;

		using namespace game;

		while (token = m_lexer.nextToken())
		{
			if (*token == TokenType::End || *token == TokenType::CurlyBracketLeft || *token == TokenType::SquareBracketRight) //if statments are closed by a begining codeblock
			{
				m_lexer.prev();
				break;
			}
			//deduct type
			else if (*token == TokenType::ConstInt)
			{
				game::Symbol_Core* sym = new DummyInt(m_lexer.getInt(*token));
				dummySymbols.emplace_back(sym);
				outputQue.push_back(sym);
			}
			else if (*token == TokenType::ConstFloat)
			{
				game::Symbol_Core* sym = new DummyFloat(m_lexer.getFloat(*token));
				dummySymbols.emplace_back(sym);
				outputQue.push_back(sym);
			}
			else if (*token == TokenType::ConstStr)
			{
				m_gameData.m_internStrings.emplace_back();

				m_gameData.m_internStrings.back().value.emplace_back(m_lexer.getWord(*token));

				outputQue.push_back(&m_gameData.m_internStrings.back());
			}
			else if (*token == TokenType::Symbol)
			{
				//find a symbol with the given name
				string str = m_lexer.getWord(*token);
				int i;

				//known constants are valid anywhere
				if ((i = m_gameData.m_constFloats.find(str)) != -1)
				{
					game::Symbol_Core* sym = new DummyFloat(m_gameData.m_constFloats[i].value[0]);
					dummySymbols.emplace_back(sym);
					outputQue.push_back(sym);
				}
				else if ((i = m_gameData.m_constInts.find(str)) != -1)
				{
					//	outputQue.emplace_back(m_gameData.m_constInts[i]);
					game::Symbol_Core* sym = new DummyInt(m_gameData.m_constInts[i].value[0]);
					dummySymbols.emplace_back(sym);
					outputQue.push_back(sym);
				}
				else if (_function)
				{
					if ((i = _function->locals.find(str)) != -1)
					{
						outputQue.push_back(&_function->locals[i]);
					}
					else if ((i = _function->params.find(str)) != -1)
					{
						outputQue.push_back(&_function->params[i]);
					}
					else if ((i = m_gameData.m_symbols.find(str)) != -1)
					{
						outputQue.push_back(&m_gameData.m_symbols[i]);
					}
					//functions go to the stack first when it is a call
					else if ((i = m_gameData.m_functions.find(str)) != -1)
					{
						Token* tokenOpt;
						// a function call
						if (TOKENOPT(TokenType::ParenthesisLeft))
						{
							functionSymbols.push_back(&m_gameData.m_functions[i]);
							operatorStack.push_back(new MathSymbol(((int)functionSymbols.size() - 1 + 11) * -1, *token));

							operatorStack.emplace_back((MathSymbol*)nullptr);
						}
						//function as param
						else
						{
							outputQue.push_back(&m_gameData.m_functions[i]);

							m_lexer.prev();
						}
					}
					else if ((i = m_gameData.m_instances.find(str)) != -1)
					{
						outputQue.push_back(&m_gameData.m_instances[i]);
					}
					//const strings are not parsed by Term()
					//thus they can only be found in a codesegment
					else if ((i = m_gameData.m_constStrings.find(str)) != -1)
					{
						outputQue.push_back(&m_gameData.m_constStrings[i]);
					}
					else if (m_currentNamespace && ((i = m_currentNamespace->elem.find(str)) != -1))
					{
						outputQue.push_back(&(m_currentNamespace->elem[i]));
					}
					// inside a file declarations can apear after a symbol is used
					else
					{
						m_undeclaredSymbols.emplace(str, *token);
						outputQue.push_back(&m_undeclaredSymbols.back());
					}
				}
				else PARSINGERROR("Unknown symbol or not a const expression.", token);
			}
			//operators to the operatorstack
			else if ((*token).type == TokenType::Operator)
			{
				//convert to a Operator Index
				for (int i = 0; i < lang::operatorCount; ++i)
				{
					const lang::Operator& o1 = lang::operators[i];
					if (m_lexer.compare(*token, o1.op))
					{
						//stack not empty and operator on top
						while (operatorStack.size() && operatorStack.back() && operatorStack.back()->value >= 0 &&
							((o1.associativity == lang::LtR && o1.precedence >= lang::operators[operatorStack.back()->value].precedence)
							|| (o1.associativity == lang::RtL && o1.precedence > lang::operators[operatorStack.back()->value].precedence))) //RtL is implicit when not LtR
						{
							auto dummy = operatorStack.back();
							dummySymbols.emplace_back(dummy);

							outputQue.push_back(dummy);
							operatorStack.pop_back();
						}
						operatorStack.emplace_back(new MathSymbol(i, *token));
						break;
					}
				}
			}
			//comma function param seperator
			else if (*token == TokenType::Comma)
			{
				while (operatorStack.back())
				{
					auto dummy = operatorStack.back();
					dummySymbols.emplace_back(dummy);

					outputQue.push_back(dummy);

					operatorStack.pop_back();

					if (!operatorStack.size()) PARSINGERROR("Parenthese mismatch.", token);
				}
			}
			//parenthesis
			else if (*token == TokenType::ParenthesisLeft)
			{
				operatorStack.emplace_back((MathSymbol*)nullptr);
			}
			else if (*token == TokenType::ParenthesisRight)
			{
				while (operatorStack.size() && operatorStack.back() != nullptr)
				{
					auto dummy = operatorStack.back();
					dummySymbols.emplace_back(dummy);

					outputQue.push_back(dummy);

					operatorStack.pop_back();
				}
				//no parenthese open found
				if (!operatorStack.size()) PARSINGERROR("Parenthese mismatch.", token);

				//pop the left parenthese
				operatorStack.pop_back();

				//check for a function
				if (operatorStack.size() && operatorStack.back()->value < -10)
				{
					auto dummy = operatorStack.back();
					dummySymbols.emplace_back(dummy);

					outputQue.push_back(dummy);

					operatorStack.pop_back();
				}
			}
			//array var
			else if (*token == TokenType::SquareBracketLeft)
			{
				m_lexer.prev();
				m_lexer.prev();

				//the previous token must allow for array access
				par::Token& peek = *m_lexer.nextToken();
				if (!(peek == TokenType::Symbol)) PARSINGERROR("This symbol does not support array like access.", &peek);

				m_lexer.nextToken();//discard the SquareBracket
				//
				game::DummyInt result(0);

				if (Term(&result)) PARSINGERROR("Term does not resolve to an int.", token);

				TOKEN(SquareBracketRight);

				game::Symbol& symbol = *(game::Symbol*)outputQue.back();
				outputQue.pop_back(); //will be substituted by the ArraySymbol
				if (symbol.size < 2) PARSINGERROR("Not an array.", &peek);

				ArraySymbol* arraySym = new ArraySymbol(symbol, result.value);
				dummyArraySymbols.emplace_back(arraySym);
				outputQue.push_back(arraySym);
			}
		}
		while (operatorStack.size() && (operatorStack.back() != nullptr))
		{
			auto sym = operatorStack.back();
			dummySymbols.emplace_back(sym);
			outputQue.push_back(sym);

			operatorStack.pop_back();
		}
		if (operatorStack.size()) PARSINGERROR("Parenthese mismatch.", token);


		//calculate value
		//translate to bytecode
		auto it = outputQue.begin();
		while (outputQue.size() > 1)
		{
			//search for the next operator
			while (!(*it)->isOperator) it++;

			MathSymbol& op = *((MathSymbol*)(*it));

			if (op.value < -10)
			{
				Symbol_Function& func = *functionSymbols[op.value * -1 - 11];

				op.param.resize(func.params.size());

				auto paramStack = it;

				//check params
				for (int i = (int)func.params.size() - 1; i >= 0; --i)
				{
					paramStack--;
					
					if (!verifyParam(func.params[i], *(*paramStack)))
						PARSINGERROR("No overload with the given arguments found.", &op.token);;
				}

				auto firstParam = paramStack;
				//push params left to right
				for (int i = 0; i < func.params.size(); ++i)
				{
					op.param[i] = *paramStack;
				//	pushParamInstr(*paramStack, _function->byteCode);
					paramStack++;
				}
				op.stackInstruction.emplace_back(func.testFlag(game::Flag::External) ? game::callExtern : game::call, func.id);

				outputQue.erase(firstParam, it);
				(*it)->type = func.returnType;
			}
			//operator
			else
			{
				auto& operatorType = lang::operators[op.value];

				//inc/dec and signs +/- have less params
				//but still use lambdas with two params
				auto param0 = it; 
				if(operatorType.paramCount > 0) param0--;

				auto param1 = param0; 
				if (operatorType.paramCount > 1) param1--;

				//params are only pushed to the stack in non const expressions

				auto returnSym = lang::operators[op.value].toByteCode(*param1, *param0, &op.stackInstruction);
				if (!returnSym)
					int uodu = 1;
				if (!returnSym) PARSINGERROR("No operator for the given arguments found.", &op.token);

				// a const value that can be substituted
				if (returnSym->type == 8 || returnSym->type == 9)
				{
					(*it) = returnSym;

					dummySymbols.emplace_back(*it);
				}
				//otherwise just save the returned type
				else
				{
					(*it)->type = returnSym->type;
					op.param.resize(operatorType.paramCount);

					if (lang::operators[op.value].associativity == lang::LtR)
					{
						param0 = param1;
						for (int i = 0; i < operatorType.paramCount; ++i)
						{
							op.param[i] = *param0;
							param0++;
						}
					}
					else //assignment takes its params right to left
					{
						param0 = it;

						for (int i = 0; i < operatorType.paramCount; ++i)
						{
							param0--;
							op.param[i] = *param0;
						}
					}

					//symbol not required anymore
					delete returnSym;
				}

				outputQue.erase(param1, it);
			}
			//it is still pointing to the processed operator
			it++;
		}


		if (_ret)
		{
			auto* result = *outputQue.begin();

			if (_ret->type == 8)
			{
				DummyInt& retInt = *(DummyInt*)_ret;

				if (result->type == 8)
					retInt.value = (*(DummyInt*)result).value;
				else if (result->type == 9)
					retInt.value = (int)(*(DummyFloat*)result).value;
			}
			else if (_ret->type == 9)
			{
				DummyFloat& retFloat = *(DummyFloat*)_ret;

				if (result->type == 8)
					retFloat.value = (float)(*(DummyInt*)result).value;
				else if (result->type == 9)
					retFloat.value = (*(DummyFloat*)result).value;
			}
			//just return the type
			else
			{
				_ret->type = result->type;
			}

		}

		if(instrStack) pushInstr(*outputQue.begin(), *instrStack);

		return 0;
	}

	// ***************************************************** //

	int Parser::pushParamInstr(game::Symbol_Core* _sym, std::vector< game::StackInstruction >& _instrStack)
	{
		//array
		if (_sym->type == 2)
		{
			game::Symbol& sym = *(game::Symbol*)_sym;
			if (sym.size > 1)
			{
				_instrStack.emplace_back(game::Instruction::pushArray, ((game::Symbol*)_sym)->id);
				_instrStack.push_back((game::Instruction)((par::ArraySymbol*)_sym)->index);

				return 0;
			}
		}

		switch (_sym->type)
		{
		case 2:
			_instrStack.emplace_back(game::Instruction::pushVar, ((game::Symbol*)_sym)->id);
			break;

		case 3:
			_instrStack.emplace_back(game::Instruction::pushVar, ((game::Symbol*)_sym)->id);
			break;

		case 5: _instrStack.emplace_back(((game::Symbol*)_sym)->testFlag(game::Const) ? game::Instruction::pushInt : game::pushVar,
			((game::Symbol*)_sym)->id);
			break;

		case 7:
			_instrStack.emplace_back(game::Instruction::pushInst, ((game::Symbol*)_sym)->id);
			break;

		case 8:
			_instrStack.emplace_back(game::Instruction::pushInt, ((game::DummyInt*)_sym)->value);
			break;
		default: 
			_instrStack.emplace_back(game::Instruction::pushInst, ((game::Symbol*)_sym)->id);
			//return -1;
		}

		return 0;
	}

	// ***************************************************** //

	int Parser::parseCodeBlock(par::Token& _token, game::Symbol_Function& _functionSymbol)
	{
		Token* tokenOpt;

		if (_token != CurlyBracketLeft) PARSINGERROR("Expected '{'.", &_token);

		while (TOKENOPT(Symbol))
		{
			//local var
			if (m_lexer.compare(*tokenOpt, "var"))
			{
				declareVar(false, _functionSymbol.locals);
			}
			else if (m_lexer.compare(*tokenOpt, "if"))
			{
				Term(nullptr, &_functionSymbol);

				_functionSymbol.byteCode.push_back(game::Instruction::jmpf);

				int jmpPos = _functionSymbol.byteCode.size() - 1;

				parseCodeBlock(_functionSymbol);

				//jump points towards the instruction after the conditional code block
				_functionSymbol.byteCode[jmpPos].param = _functionSymbol.byteCode.size();
			}
			else if (m_lexer.compare(*tokenOpt, "return"))
			{
				game::Symbol_Core returnSymbol;

				Term(&returnSymbol, &_functionSymbol);
				TOKEN(End);

				if (returnSymbol.type != _functionSymbol.returnType) PARSINGERROR("Type mismatch.", tokenOpt);

				_functionSymbol.byteCode.push_back(game::Instruction::Ret);
			}
			else
			{
				//term is analysed completly, first symbol is not a keyword
				m_lexer.prev();

				Term(nullptr, &_functionSymbol);
				TOKEN(End);
			}
		}

		if (*tokenOpt != CurlyBracketRight) PARSINGERROR("Expected token '}'.", tokenOpt);

		return 0;
	}

	// ***************************************************** //

	int Parser::pushInstr(game::Symbol_Core* _sym, std::vector< game::StackInstruction >& _instrStack)
	{
		if (!_sym->isOperator)
		{
			pushParamInstr(_sym, _instrStack);
		}
		else
		{
			MathSymbol& symbol = *(MathSymbol*)_sym;

			for (int i = 0; i < symbol.param.size(); ++i)
				pushInstr(symbol.param[i], _instrStack);

			//add the operator instruction(s)
			_instrStack.insert(_instrStack.end(), symbol.stackInstruction.begin(), symbol.stackInstruction.end());
		}

		return 0;
	}

	// ***************************************************** //

	void Parser::assignFunctionParam(game::Symbol_Function& _func)
	{
		for (int i = _func.params.size() - 1; i >= 0; --i)
		{
			game::Instruction assignInstr;
			game::Instruction pushInstr;

			switch (_func.params[i].type)
			{
				//floats and ints are handled the same in gbc
			case 1: assignInstr = game::Instruction::assign;
				pushInstr = game::Instruction::pushVar;
			case 2: assignInstr = game::Instruction::assign;
				pushInstr = game::Instruction::pushVar;
				break;
			case 3: assignInstr = game::Instruction::assignStr;
				pushInstr = game::Instruction::pushVar;
				break;
			default: assignInstr = game::Instruction::assignInst;
				pushInstr = game::Instruction::pushInst;
			}

			_func.byteCode.emplace_back(pushInstr, _func.params[i].id);
			_func.byteCode.emplace_back(assignInstr);
		}
	}

	// ***************************************************** //

	bool Parser::verifyParam(game::Symbol& _expected, game::Symbol_Core& _found)
	{
		//float params
		if (_expected.type == 1 && (_found.type == 8 || _found.type == 9))
		{
			//dirty hack
			//results into a pointer typecast of the value
			_found.type = 8;

			return true;
		}
		//int params can be filled by a const int aswell
		//default instance(7) is valid with any kind of instance params
		else if (_expected.type != _found.type && (_expected.type != 2 || _found.type != 8)
			&& (_expected.type != 7 || _found.type <= 10))
			return false;
		return true;
	}

}
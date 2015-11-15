#include "easylogging++.h"

#include "parser.h"
#include "reservedsymbols.h"
#include "utils.h"

using namespace std;

namespace par{


	int Parser::Term(game::Symbol_Core* _ret, game::Symbol_Function* _function, TokenType _endChar)
	{
		game::ByteCodeStack* instrStack = _function ? &_function->byteCode : nullptr;

		//Shunting-yard algorithm
		//http://en.wikipedia.org/wiki/Shunting-yard_algorithm

		//inlined symbols
		std::vector < std::unique_ptr < game::Symbol_Core > > dummySymbols;
		//somehow destruction goes wrong when a game::Symbol is added to dummySymbols
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

		while (token = m_lexer->nextToken())
		{
			if (*token == _endChar)
			{
				m_lexer->prev();
				break;
			}
			//deduct type
			else if (*token == TokenType::ConstInt)
			{
				game::Symbol_Core* sym = new DummyInt(m_lexer->getInt(*token));
				dummySymbols.emplace_back(sym);
				outputQue.push_back(sym);
			}
			else if (*token == TokenType::ConstFloat)
			{
				game::Symbol_Core* sym = new DummyFloat(m_lexer->getFloat(*token));
				dummySymbols.emplace_back(sym);
				outputQue.push_back(sym);
			}
			else if (*token == TokenType::ConstStr)
			{
				game::ConstSymbol_String* constStr = new game::ConstSymbol_String();
				m_gameData.m_internStrings.emplace_back(constStr);
				constStr->value[0] = m_lexer->getWord(*token);
				outputQue.push_back(constStr);
			}
			else if (*token == TokenType::Symbol)
			{
				//find a symbol with the given name
				string str = m_lexer->getWord(*token);
				int i;
				//known constants are valid anywhere
				if ((i = utils::find(m_gameData.m_constSymbols, str)) != -1)
				{
					//inline them by pushing just the data to the que
					game::Symbol_Core* sym;
					switch (m_gameData.m_constSymbols[i].type)
					{
					case 1:
						sym = new DummyFloat(((ConstSymbol_Float&)m_gameData.m_constSymbols[i]).value[0]);
						break;
					case 2:
						sym = new DummyInt(((ConstSymbol_Int&)m_gameData.m_constSymbols[i]).value[0]);
						break;
					}
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
						// function could be a call
						//that is handled differently
						if (m_gameData.m_symbols[i].type == 5)
						{
							game::Symbol_Function& symbol = (game::Symbol_Function&) m_gameData.m_symbols[i];
							Token* tokenOpt;
							// followed by a '('
							if (TOKENOPT(TokenType::ParenthesisLeft))
							{
								functionSymbols.push_back(&symbol);
								auto* mathToken = new MathSymbol(((int)functionSymbols.size() - 1 + 11) * -1, *token);

								//functions without param do not get an type update
								mathToken->type = symbol.returnType;
								operatorStack.push_back(mathToken);

								//push the parenthese to save one iteration
								operatorStack.emplace_back((MathSymbol*)nullptr);

								continue;
							}
							else m_lexer->prev();
						}

						outputQue.push_back(&m_gameData.m_symbols[i]);
					}
					else if (m_currentNamespace && ((i = m_currentNamespace->elem.find(str)) != -1))
					{
						outputQue.push_back(&(m_currentNamespace->elem[i]));
					}
					//a member var with a '.' int front of it
					else if (operatorStack.size() && operatorStack.back() && operatorStack.back()->value == 2)
					{
						size_t j = outputQue.back()->type;
						i = m_gameData.m_types[j].elem.find(str);
						if (i != -1)
						{
							//the correct this pointer is set by pushParamInstr
							//remove the instance name
							auto& instance = *(game::Symbol*)outputQue.back();
							outputQue.pop_back();

							//exchange it for a dummy
							game::Symbol_Core* sym = new DummyInstance(&instance);
							dummySymbols.emplace_back(sym);
							outputQue.push_back(sym);

							outputQue.push_back(&(m_gameData.m_types[j].elem[i]));
						}
						else PARSINGERROR(m_gameData.m_types[j].name + " has no member with this name.", token);
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
					if (m_lexer->compare(*token, o1.op))
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
				if (operatorStack.size() && operatorStack.back() && operatorStack.back()->value < -10)
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
				m_lexer->prev();
				m_lexer->prev();

				//the previous token must allow for array access
				par::Token& peek = *m_lexer->nextToken();
				if (!(peek == TokenType::Symbol)) PARSINGERROR("This symbol does not support array like access.", &peek);

				m_lexer->nextToken();//discard the SquareBracket

				game::DummyInt result(0);

				if (Term(&result, TokenType::SquareBracketRight)) PARSINGERROR("Term does not resolve to an int.", token);

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
		//a single operand needs to be processed(like a call: void())
		//while a single var is just forwarded
		if (outputQue.size() > 1 || (*it)->isOperator)
		do
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
						PARSINGERROR("No overload with the given arguments found.", &op.token);
				}

				auto firstParam = paramStack;
				//push params left to right
				for (int i = 0; i < func.params.size(); ++i)
				{
					op.param[i] = *paramStack;
				//	pushParamInstr(*paramStack, _function->byteCode);
					paramStack++;
				}
				op.stackInstruction.emplace_back(func.testFlag(game::Flag::External) ? game::callExtern : game::call, &func);

				outputQue.erase(firstParam, it);
				//(*it)->type = func.returnType;
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

					//member access is the only operator that is handled ltr
					if (operatorType.op == ".")
					{
						param0 = param1;
						for (int i = 0; i < operatorType.paramCount; ++i)
						{
							op.param[i] = *param0;
							param0++;
						}
					}
					else //assignment takes its params right to left ; actually every regular operator
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
		} while (outputQue.size() > 1);


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

	int Parser::pushParamInstr(game::Symbol_Core* _sym, game::ByteCodeStack& _instrStack)
	{
		//consts

		if (_sym->type == 8 || _sym->type == 9)
		{
			_instrStack.emplace_back(game::Instruction::pushInt, ((game::DummyInt*)_sym)->value);
			return 0;
		}
		// class member
		else if (_sym->type == 11)
		{
			game::Symbol* instancePtr = ((game::DummyInstance*)_sym)->value;
			_instrStack.emplace_back(game::Instruction::setInst, instancePtr);
			m_thisInst = instancePtr;
			return 0;
		}

		game::Symbol& sym = *(game::Symbol*)_sym;

		//array
		if (_sym->type == 2)
		{
			if (sym.size > 1)
			{
				_instrStack.emplace_back(game::Instruction::pushArray, ((par::ArraySymbol*)_sym)->symbol);
				_instrStack.emplace_back((game::Instruction)((par::ArraySymbol*)_sym)->index);

				return 0;
			}
		}

		switch (_sym->type)
		{
		case 2:
			_instrStack.emplace_back(game::Instruction::pushVar, ((game::Symbol*)_sym));
			break;

		case 3:
			_instrStack.emplace_back(game::Instruction::pushVar, ((game::Symbol*)_sym));
			break;

		case 5: _instrStack.emplace_back(((game::Symbol*)_sym)->testFlag(game::Const) ? game::Instruction::pushInt : game::pushVar,
			((game::Symbol*)_sym));
			break;

		case 7:
			_instrStack.emplace_back(game::Instruction::pushInst, ((game::Symbol*)_sym));
			break;
		default: 
			_instrStack.emplace_back(game::Instruction::pushInst, ((game::Symbol*)_sym));
			//return -1;
		}

		return 0;
	}

	// ***************************************************** //

	int Parser::codeBlock(par::Token& _token, game::Symbol_Function& _functionSymbol)
	{
		Token* tokenOpt;

		//keep reference for later
		m_codeQue->emplace_back(m_lexer->tokenToIterator(_token), _functionSymbol, m_currentNamespace);

		//skip to the end of the codeblock
		Token* token = &_token;
		unsigned int bracketCount = 0;

		do
		{
			if (token->type == CurlyBracketLeft) bracketCount++;
			else if (token->type == CurlyBracketRight) bracketCount--;
			token = m_lexer->nextToken();
		} while (bracketCount > 0);

		//jumped over the next token
		m_lexer->prev();
		SEMIKOLON;

		return 0;
	}

	// ***************************************************** //

	int Parser::parseCodeBlock(CodeToParse& _codeToParse)
	{
		//restore envoirement of the block
		m_lexer->setTokenIt(_codeToParse.m_tokenIt);
		m_currentNamespace = _codeToParse.m_namespace;

		parseCodeBlock(_codeToParse.m_function);

		return 0;
	}

	// ***************************************************** //

	int Parser::parseCodeBlock(game::Symbol_Function& _functionSymbol)
	{
		TOKEN(CurlyBracketLeft);

		Token* tokenOpt;

		while (TOKENOPT(Symbol))
		{
			//local var
			if (m_lexer->compare(*tokenOpt, "var"))
			{
				declareVar(false, _functionSymbol.locals);
			}
			else if (m_lexer->compare(*tokenOpt, "if"))
			{
				conditionalBlock(_functionSymbol);
			}
			else if (m_lexer->compare(*tokenOpt, "return"))
			{
				game::Symbol_Core returnSymbol;

				Term(&returnSymbol, &_functionSymbol);
				TOKEN(End);

				if (!verifyParam(_functionSymbol.returnType, returnSymbol.type)) PARSINGERROR("Type mismatch.", tokenOpt);

				_functionSymbol.byteCode.emplace_back(game::Instruction::Ret);
			}
			else
			{
				//term is analysed completly, first symbol is not a keyword
				m_lexer->prev();

				Term(nullptr, &_functionSymbol);
				TOKEN(End);
			}
		}

		if (*tokenOpt != CurlyBracketRight) PARSINGERROR("Expected token '}'.", tokenOpt);

		SEMIKOLON;

		//reset the thisInst
		m_thisInst = nullptr;

		return 0;
	}

	// ***************************************************** //

	int Parser::conditionalBlock(game::Symbol_Function& _functionSymbol)
	{
		std::vector < size_t > endJumps; //list of jumps that should lead to the end of the conditional block

		//initial if statement
		m_lexer->prev();

		size_t condJmp;
		//aditional "else" statements
		do 
		{
			if (m_lexer->compare(*(m_lexer->peek()), "if"))
			{
				m_lexer->nextToken();

				Term(nullptr, &_functionSymbol, TokenType::CurlyBracketLeft);

				_functionSymbol.byteCode.emplace_back(game::Instruction::jmpf, 0);
				condJmp = _functionSymbol.byteCode.size() - 1;

				parseCodeBlock(_functionSymbol);

				//jump points towards the instruction after this code block
				// + the offset for a jump instruction
				_functionSymbol.byteCode[condJmp].param = _functionSymbol.byteCode.getStackSize() + 5;
			}
			else
			{
				parseCodeBlock(_functionSymbol);
				condJmp = 0xFFFFFFFF;
			}

			//jump towards the end of the conditional block
			_functionSymbol.byteCode.emplace_back(game::Instruction::jmp, 0);
			endJumps.push_back(_functionSymbol.byteCode.size() - 1);

		} while (m_lexer->compare(*m_lexer->nextToken(), "else"));

		//the last codeblock does not need a jump to reach the end
		_functionSymbol.byteCode.pop_back();
		//last block was "if else" and has its end jump point wrong
		if (condJmp != 0xFFFFFFFF)_functionSymbol.byteCode[condJmp].param.sadr -= 5; 

		//set the endJumps
		size_t nextInstruction = _functionSymbol.byteCode.getStackSize();
		for (size_t i = 0; i < endJumps.size() - 1; ++i)
			_functionSymbol.byteCode[endJumps[i]].param = nextInstruction;

		m_lexer->prev();

		return 0;
	}

	// ***************************************************** //

	int Parser::pushInstr(game::Symbol_Core* _sym, game::ByteCodeStack& _instrStack)
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

		//	_instrStack.reserve(_instrStack.size() + symbol.stackInstruction.size());
			//add the operator instruction(s)
			for (auto& stackInstr : symbol.stackInstruction)
			{
				_instrStack.push_back(stackInstr);
			}
		}

		return 0;
	}

	// ***************************************************** //

	void Parser::assignFunctionParam(game::Symbol_Function& _func)
	{
		for (int i = (int)_func.params.size() - 1; i >= 0; --i)
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

			_func.byteCode.emplace_back(pushInstr, &_func.params[i]);
			_func.byteCode.emplace_back(assignInstr);
		}
	}

	// ***************************************************** //

	bool Parser::verifyParam(int _expected, int _found)
	{
		if (_expected == _found) return true;
		//float params
		else if (_expected == 1 && (_found == 8 || _found == 9))
		{
			//dirty hack
			//results into a pointer typecast of the value
			_found = 8;

			return true;
		}
		//int params can be filled by a const int and instances
		else if (_expected == 2 && (_found == 8 || _found == 7 || _found >= g_atomTypeCount)) return true;
		//and any instance
		//default instance(7) is valid with any kind of instance params
		//aswell as 
		else if (_expected == 7 && (_found >= g_atomTypeCount || _found == 11)) return true;

		return false;
	}

}
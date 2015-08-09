#pragma once

#include "symboltable.h"

namespace lang{
	//this may not be changed
	//gothic runs with this indizies
	std::initializer_list < game::Symbol_Type > basicTypes = { 
		game::Symbol_Type(std::string("void")),				//0
		game::Symbol_Type(std::string("float")),			//1
		game::Symbol_Type(std::string("int")),				//2
		game::Symbol_Type(std::string("string")),			//3
		game::Symbol_Type(std::string("class")),			//4
		game::Symbol_Type(std::string("func")),				//5
		game::Symbol_Type(std::string("prototype")),		//6
		game::Symbol_Type(std::string("instance")),			//7
		//this are interns used by the parser only
		//symbols of this type are completly inlined
		game::Symbol_Type(std::string("constInt")),			//8
		game::Symbol_Type(std::string("constFloat")),		//9
		game::Symbol_Type(std::string("operator"))			//10
		//starting from 11 custom types have their place
	};


} //end namespace
// deadalus_pp_parser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "parser.h"
#include <stdio.h>
#include <conio.h>
#include <time.h>

#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
/*	string test = "blub";
	unsigned int ptr[sizeof(string) / sizeof(int)];
	memcpy(&ptr, &test, sizeof(string));
	*/

	/* vs std::string layout
	+0 _Myproxy
	+4 _Ptr
	+8 _Myfirst
	+12 _Mylast
	+16 _Buf / Myend
	+20	_Mysize
	+24 _Myres (capacity)
	*/

	//C:\Spiele\Gothic II\_work\data\SCRIPTS\Content

	par::Parser parser("config.json");

	clock_t begin = clock();

	if(parser.parse("gothic.src"))
		parser.compile();
	
	clock_t end = clock();

	printf(("The whole proccess took: " + std::to_string(double(end - begin) / CLOCKS_PER_SEC) + "sec").c_str());

	//dont close automatically
	printf("\nPress a key to continue...");
	int c = _getch();

	return 0;
}


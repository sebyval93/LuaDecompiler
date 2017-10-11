#include "decompiler.h"
#include <iostream>

int main(int argc, const char* argv[])
{
	Decompiler dec;

	if (argc < 2)
	{
		std::cout << "Usage: LuaDecompiler file or folder path(s)";
	}
	else
	{
		for (int i = 1; i < argc; ++i)
		{
			dec.processPath(argv[i]);
		}

		std::cout << "\nDone!\n";
	}

	char c;
	std::cin >> c;
}
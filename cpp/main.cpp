// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

#include "gcs.h"
#include <iostream>
#include <fstream>

static void usage(void)
{
	std::cerr << "Usage:\n";
	std::cerr << "   gcs build /path/to/wordlist\n";
	std::cerr << "   gcs query word1 [word2...]\n";
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		usage();
		return 1;
	}

	std::string cmd(argv[1]);
	if (cmd == "build")
	{
		if (argc != 3)
		{
			usage();
			return 1;
		}

		std::ifstream f(argv[2], std::ios::binary|std::ios::in);
		if (!f.is_open())
		{
			std::cerr << "Cannot open file: " << argv[2] << "\n";
			return 1;
		}

		int numwords = 0;
		std::string line;

		while (std::getline(f,line), !f.eof())
			++numwords;
		f.clear();
		f.seekg(0);
		std::cout << "numwords: " << numwords << "\n";

		GCSBuilder gcs(numwords, 1024);
		while (std::getline(f,line), !f.eof())
			gcs.add(line.data(), line.size());
		f.close();

		std::ofstream out("table.gcs", std::ios::binary|std::ios::out);
		if (!out.is_open())
		{
			std::cerr << "Cannot open output file: " << "table.gcs" << "\n";
			return 1;
		}
		gcs.finalize(out);
		out.close();
	}
	else if (cmd == "query")
	{
		if (argc < 3)
		{
			usage();
			return 1;
		}	

		std::ifstream f("table.gcs", std::ios::binary|std::ios::in);
		if (!f.is_open())
		{
			std::cerr << "Cannot open table: " << "table.gcs" << "\n";
			return 1;
		}

		GCSQuery q(f);
		for (int i=2; i < argc; ++i)
		{
			std::string s(argv[i]);
			bool found = q.query(s.data(), s.size());

			std::cout << "Querying for \"" << s << "\": " <<
				(found ? "TRUE" : "FALSE") << "\n";
		}
	}
	else
	{
		std::cerr << "Invalid command: " << cmd << "\n";
		usage();
		return 1;
	}

	return 0;
}

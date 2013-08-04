#pragma once

#include <vector>
#include <iostream>
#include <fstream>

using namespace std;
namespace DePlaguarism{

	class ShingleAppLogger
	{
	private:
		vector<ostream*> targets; // streams for output, first MUST be cout
	public:
		ShingleAppLogger();
		void addTrgt(ostream * src);
		void addLogFile(char * fileName);
		~ShingleAppLogger();
        void flush();
		ShingleAppLogger & operator<<(char * item);
		ShingleAppLogger & operator<<(char item);
		ShingleAppLogger & operator<<(float item);
		ShingleAppLogger & operator<<(string item);
		ShingleAppLogger & operator<<(int item);
        ShingleAppLogger & operator<<(unsigned int item);
	};
}

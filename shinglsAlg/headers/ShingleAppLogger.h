#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <time.h>

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
		ShingleAppLogger & operator<<(char * item);
		ShingleAppLogger & operator<<(char item);
		ShingleAppLogger & operator<<(float item);
		ShingleAppLogger & operator<<(string item);
		ShingleAppLogger & operator<<(int item);
	};
}
#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <time.h>

using namespace std;

class ShingleAppLogger
{
private:
	vector<ostream*> targets; // streams for output, first MUST be cout
public:
	ShingleAppLogger();
	void addTrgt(ostream * src);
	void addLogFile(char * fileName);

	ShingleAppLogger toLog(char * item);
	ShingleAppLogger toLog(int item);
	ShingleAppLogger toLog(float item);
	ShingleAppLogger toLog(string item);
	ShingleAppLogger toLogIp(unsigned long ip);
	ShingleAppLogger toLogNow();
	ShingleAppLogger endLine();

	~ShingleAppLogger();
};


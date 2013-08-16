#ifndef SHINGLE_APP_LOGGER_H
#define SHINGLE_APP_LOGGER_H

#include "./precompiled.h"

using namespace std;
namespace DePlaguarism{

	class ShingleAppLogger
	{
	private:
		vector<ostream*> targets; // streams for output, first MUST be cout
	public:
		ShingleAppLogger();
		void addTrgt(ostream * src);
        void addLogFile(const char * fileName);
		~ShingleAppLogger();
        void flush();
        ShingleAppLogger & operator<<(const char * item);
        ShingleAppLogger & operator<<(char item);
		ShingleAppLogger & operator<<(float item);
        ShingleAppLogger & operator<<(const string & item);
		ShingleAppLogger & operator<<(int item);
        ShingleAppLogger & operator<<(unsigned int item);
	};
}

#endif

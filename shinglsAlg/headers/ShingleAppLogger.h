#ifndef SHINGLE_APP_LOGGER_H
#define SHINGLE_APP_LOGGER_H

#include "./precompiled.h"

namespace DePlaguarism{

    class ShingleAppLogger
    {
    private:
        std::vector<std::ostream*> targets; // streams for output, first MUST be cout
    public:
        ShingleAppLogger();
        void addTrgt(std::ostream * src);
        void addLogFile(const char * fileName);
        ~ShingleAppLogger();
        void flush();
        ShingleAppLogger & operator<<(const char * item);
        ShingleAppLogger & operator<<(char item);
        ShingleAppLogger & operator<<(float item);
        ShingleAppLogger & operator<<(const std::string & item);
        ShingleAppLogger & operator<<(int item);
        ShingleAppLogger & operator<<(unsigned int item);
    };
}

#endif

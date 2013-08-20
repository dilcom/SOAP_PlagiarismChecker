#include "../headers/ShingleAppLogger.h"

using namespace DePlaguarism;
ShingleAppLogger::ShingleAppLogger(){
    targets.push_back(&cout);
}

ShingleAppLogger::~ShingleAppLogger(){
    for (vector<ostream*>::iterator it = targets.begin() + 1; it != targets.end(); it++)
        delete *it;
}

void ShingleAppLogger::addTrgt(ostream * src){
    if (src->good())
        targets.push_back(src);
}

void ShingleAppLogger::addLogFile(const char * filename){
    ofstream * src = new ofstream();
    src->open(filename, ios::out | ios::app);
    addTrgt(src);
}

void ShingleAppLogger::flush(){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        **it << std::flush;
}

ShingleAppLogger & ShingleAppLogger::operator<<(const char * item){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        **it << item;
    return *this;
}

ShingleAppLogger & ShingleAppLogger::operator<<(float item){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        **it << item;
    return *this;
}

ShingleAppLogger & ShingleAppLogger::operator<<(int item){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        **it << item;
    return *this;
}

ShingleAppLogger & ShingleAppLogger::operator<<(unsigned int item){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        **it << item;
    return *this;
}

ShingleAppLogger & ShingleAppLogger::operator<<(const string & item){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        (**it).write(item.data(), item.size());
    return *this;
}

ShingleAppLogger & ShingleAppLogger::operator<<(char item){
    for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
        **it << item;
    return *this;
}

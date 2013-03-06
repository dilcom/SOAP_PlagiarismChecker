#include "../headers/stdafx.h"
#include "../headers/ShingleAppLogger.h"

using namespace DePlaguarism;
ShingleAppLogger::ShingleAppLogger(){
}

ShingleAppLogger::~ShingleAppLogger(){
    //for (vector<ostream*>::iterator it = targets.begin() + 1; it != targets.end(); it++)
    //	delete *it;
}

void ShingleAppLogger::addTrgt(ostream * src){
    //if (src->good())
    //	targets.push_back(src);
}

void ShingleAppLogger::addLogFile(char * filename){
    //ofstream * src = new ofstream();
    //src->open(filename, ios::out | ios::app);
    //addTrgt(src);
}

ShingleAppLogger & ShingleAppLogger::operator<<(char * item){
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

ShingleAppLogger & ShingleAppLogger::operator<<(string item){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		(**it).write(item.data(), item.size());
	return *this;
}

ShingleAppLogger & ShingleAppLogger::operator<<(char item){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		**it << item;
	return *this;
}

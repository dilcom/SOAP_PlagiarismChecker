#include "../headers/ShingleAppLogger.h"


ShingleAppLogger::ShingleAppLogger(){
}


ShingleAppLogger::~ShingleAppLogger(){
	for (vector<ostream*>::iterator it = targets.begin() + 1; it != targets.end(); it++)
		delete *it;
}

void ShingleAppLogger::addTrgt(ostream * src){
	if (src->good())
		targets.push_back(src);
}

void ShingleAppLogger::addLogFile(char * filename){
	ofstream * src = new ofstream();
	src->open(filename, ios::out | ios::app);
	addTrgt(src);
}

ShingleAppLogger ShingleAppLogger::toLog(int item){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		**it << item;
	return *this;
}

ShingleAppLogger ShingleAppLogger::toLog(float item){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		**it << item;
	return *this;
}

ShingleAppLogger ShingleAppLogger::toLog(char * item){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		**it << item;
	return *this;
}

ShingleAppLogger ShingleAppLogger::toLog(string item){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		(**it).write(item.data(), item.length());
	return *this;
}

ShingleAppLogger ShingleAppLogger::endLine(){
	for (vector<ostream*>::iterator it = targets.begin(); it != targets.end(); it++)
		**it << endl;
	return *this;
}

ShingleAppLogger ShingleAppLogger::toLogNow(){
	time_t a;
	time(&a);
	char * str = asctime(localtime(&a));
	toLog(str);
	return *this;
}

ShingleAppLogger ShingleAppLogger::toLogIp(unsigned long ip){
	char * num = new char[4];
	char * buffer = new char[16];
	buffer[0] = '\0';
	itoa((unsigned char)(*((char*)(&ip) + 3)), num, 10);
	strcat(buffer, num);
	strcat(buffer, ".");
	itoa((unsigned char)(*((char*)(&ip) + 2)), num, 10);
	strcat(buffer, num);
	strcat(buffer, ".");
	itoa((unsigned char)(*((char*)(&ip) + 1)), num, 10);
	strcat(buffer, num);
	strcat(buffer, ".");
	itoa((unsigned char)(*((char*)(&ip) + 0)), num, 10);
	strcat(buffer, num);	
	toLog(buffer);
	delete num;
	delete buffer;
	return *this;
}
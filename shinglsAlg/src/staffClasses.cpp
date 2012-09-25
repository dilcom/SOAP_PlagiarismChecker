#include "../headers/staffClasses.h"

using namespace DePlaguarism;

TextDocument::TextDocument(const t__text & m, int num)
{
	number = num;
	this->name = m.name;
	this->authorName = m.authorName;
	this->authorGroup = m.authorGroup;
	this->data = m.streamData;
	time_t a;
	time(&a);
	this->dateTime = *(localtime(&a));
	this->type = m.type;
}
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
	this->header.dateTime = *(localtime(&a));
	this->header.type = m.type;
	this->header.authorGroup_len = authorGroup.length();
	this->header.authorName_len = authorName.length();
	this->header.data_len = data.length();
	this->header.textName_len = name.length();
}
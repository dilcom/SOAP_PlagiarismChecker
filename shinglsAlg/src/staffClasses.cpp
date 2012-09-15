#include "../headers/staffClasses.h"

using namespace shingleAppStaff;

ShingleAppText::ShingleAppText(const t__text & m){
	this->name = m.name;
	this->authorName = m.authorName;
	this->authorGroup = m.authorGroup;
	time_t a;
	time(&a);
	this->dateTime = *(localtime(&a));
	this->type = m.type;
}
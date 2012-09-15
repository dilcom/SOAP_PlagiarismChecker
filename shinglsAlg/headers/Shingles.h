#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <stddef.h>
#include <stdint.h>
#include "../include/utf8.h"
#include <vector>
#include <algorithm>
#include "constants.h"

uint_least32_t Crc32(const unsigned char * buf, size_t len); //crc32 hash function



class Shingle
{
	unsigned int data[maxShingleCount]; //array with crc32 hashes from given text
	unsigned int count; //data field length
public:
	unsigned int * getData(); // getter for data field
	unsigned int getCount(); // getter for count field
	Shingle();
	Shingle(char *txt); // contructs object from UTF-8 text data
	Shingle(int num); // constructs object from file if object is already in the base
	~Shingle();
	float compareWith(Shingle a); // compares itself with shingle "a"
	void saveToFile(int num); // saves data and count to file named "<num>article"
};



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
#include "staffClasses.h"
#include "../include/db_cxx.h"
#pragma comment (lib, "../lib/libdb53.lib")

using namespace DePlaguarism;

namespace DePlaguarism{

	class Shingle
	{
	protected:
		unsigned int data[maxShingleCount]; ///< array with crc32 hashes from given text
		unsigned int count; ///< data field length
		TextDocument textData;
	public:
		const unsigned int * getData(); ///< getter for data field
		unsigned int getCount(); ///< getter for count field
		const TextDocument & getText(); ///< getter for text field
		Shingle();
		Shingle(const t__text & txt, int num); ///< contructs object from UTF-8 text data
		~Shingle();
		void save(Db * targetDocs, Db * targetHash); ///< saves all the data to DB
	};

	uint_least32_t Crc32(const unsigned char * buf, size_t len); ///< crc32 hash function
}
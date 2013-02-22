#pragma once

#include "../include/utf8.h"
#include "constants.h"
#include "staffClasses.h"

#ifdef __WIN32__
    #include "../include/Win32/db_cxx.h"
#else
    #include "../include/UNIX/db_cxx.h"
#endif

using namespace DePlaguarism;

namespace DePlaguarism{

	class Shingle
    {
	protected:
		unsigned int data[MAX_SHINGLE_PER_TEXT]; ///< array with crc32 hashes from given text
		unsigned int count; ///< data field length
        TextDocument *textData;
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

#ifndef SHINGLES_H
#define SHINGLES_H

#include "config.h"
using namespace DePlaguarism;

namespace DePlaguarism{

    class Shingle
    {
    protected:
        unsigned int m_data[DefaultValues::MAX_SHINGLE_PER_TEXT]; ///< array with crc32 hashes from given text
        unsigned int m_count; ///< count of elements in data field
        DocHeader m_header;
        t__text *m_textData;
    public:
        const unsigned int * getData(); ///< getter for data field
        unsigned int getCount(); ///< getter for count field
        const t__text &getText(); ///< getter for text field
        Shingle();
        Shingle(const t__text &txt); ///< contructs object from UTF-8 text data
        ~Shingle();
        void save(DataSrcAbstract * targetDataSource); ///< saves all the data to DB
    };
    uint_least32_t Crc32(const unsigned char * buf, size_t len); ///< crc32 hash function
}

#endif

#ifndef SHINGLES_H
#define SHINGLES_H

#ifdef BERKELEYDB
    #include "datasrcberkeleydb.h"
#else
    #include "datasrcrediscluster.h"
#endif

namespace DePlagiarism{

    //! Represents text, its shingles and tools to get it.
    class Shingle
    {
    protected:
        unsigned int m_data[DefaultValues::MAX_SHINGLE_PER_TEXT]; ///< Array with crc32 hashes from given text
        unsigned int m_count; ///< Count of elements in data field
        DocHeader m_header; ///< Header of text stored in object
        t__text *m_textData; ///< Text itself stored here.
    public:
        const unsigned int * getData(); ///< Getter for data field
        unsigned int getCount(); ///< Getter for count field
        const t__text &getText(); ///< Getter for text field
        Shingle();
        //! Contructs object from UTF-8 text data
        /*!
           \param txt is a text with much additional information.
         */
        Shingle(const t__text &txt);
        ~Shingle();
        //! Saves all the data to DB
        /*!
         * Saves text and shingles to DB.
         * \param targetDataSource is a pointer to DB where all the information must be stored.
         */
        void save(DataSrcAbstract * targetDataSource);
    };
}

#endif

#ifndef DATASRCBERKELEYDB_H
#define DATASRCBERKELEYDB_H

#include "datasrcabstract.h"

namespace DePlaguarism{
    //! Wrpper class for Berkeley DB
    class DataSrcBerkeleyDB : public DePlaguarism::DataSrcAbstract
    {
    private:
        static DbEnv * m_env;
        static Db * m_dbSrcHashes,
        * m_dbSrcDocs;
        static unsigned int m_docNumber; ///< current document number
        static MUTEX_TYPE m_mtx;///< crossplatform mutex
        void saveDocNumber(); ///< saves current document number to DB
        bool mainClient; ///< if true, object constructs and frees static envelope and dbs
    public:
        DataSrcBerkeleyDB(const char * envName, const char * hashDbName, const char * docsDbName, bool mainFlag); ///< for single/master database
        virtual ~DataSrcBerkeleyDB();
        virtual getIdsByHashesResult__t * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text **trgtPtr, soap *parent);
    };//class header

}//namespace
#endif // DATASRCBERKELEYDB_H

#ifndef DATASRCBERKELEYDB_H
#define DATASRCBERKELEYDB_H

#include "datasrcabstract.h"
#include <memory>
#include "constants.h"
#include "../include/threads.h"
#include "../include/hiredis.h"

namespace DePlaguarism{

    class DataSrcBerkeleyDB : public DePlaguarism::DataSrcAbstract
    {
    private:
        static DbEnv * env;
        static Db * dbSrcHashes,
                  * dbSrcDocs;
        bool mainClient; ///< if true, object constructs and frees static envelope and dbs
    public:
        DataSrcBerkeleyDB(const char * envName, const char * hashDbName, const char * docsDbName, bool mainFlag); ///< for single/master database
        virtual ~DataSrcBerkeleyDB();
        virtual std::vector<unsigned int> * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void save(unsigned int docNumber, const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text **trgtPtr, soap *parent);
    };//class header

}//namespace
#endif // DATASRCBERKELEYDB_H

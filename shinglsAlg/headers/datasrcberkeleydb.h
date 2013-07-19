#ifndef DATASRCBERKELEYDB_H
#define DATASRCBERKELEYDB_H

#include "datasrcabstract.h"
#include <memory>
#include "constants.h"

namespace DePlaguarism{

    class DataSrcBerkeleyDB : public DePlaguarism::DataSrcAbstract
    {
    private:
        DbEnv * env;
        Db * dbSrc;
        bool master; ///< allows master to delete env
        DBTYPE tp;
    public:
        DataSrcBerkeleyDB(const char * dbName, const char * envName, DBTYPE dbtype, u_int32_t flag = 0); ///< for single/master database
        DataSrcBerkeleyDB(const char *dbName, DataSrcAbstract *mas, DBTYPE dbtype, u_int32_t flag = 0); ///< for slave databases (inherites env from mas)
        ~DataSrcBerkeleyDB();
        DbEnv * getEnv() { return env; }
        virtual std::vector<unsigned int> * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void saveIds(unsigned int docNumber, const unsigned int * hashes, unsigned int count);
        virtual void saveDocument(DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text *trgt, soap *parent);
    };//class header

}//namespace
#endif // DATASRCBERKELEYDB_H

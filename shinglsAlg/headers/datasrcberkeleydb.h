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
        virtual std::vector<PieceOfData> *getValues(const PieceOfData & key);
        virtual std::vector<PieceOfData> * getValues(const std::vector< PieceOfData > & keys);
        virtual void saveValue(PieceOfData * ikey, PieceOfData * idata);
    };//class header

}//namespace
#endif // DATASRCBERKELEYDB_H

#ifndef DATASRCBERKELEYDB_H
#define DATASRCBERKELEYDB_H

#include "datasrcabstract.h"
#include <memory>
#include "constants.h"

namespace Deplaguarism{

    class DataSrcBerkeleyDB : public Deplaguarism::DataSrcAbstract
    {
    private:
        DbEnv * env;
        Db * dbSrc;
        bool master; ///< allows master to delete env
        DBTYPE tp;
    public:
        DataSrcBerkeleyDB(char * dbName, char * envName, DBTYPE dbtype, u_int32_t flag = 0); ///< for single/master database
        DataSrcBerkeleyDB(char * dbName, Db * mas, DBTYPE dbtype, u_int32_t flag = 0); ///< for slave databases
        ~DataSrcBerkeleyDB();
        virtual std::shared_ptr< std::vector<PieceOfData> > getValues(const PieceOfData & key);
        virtual std::shared_ptr< std::vector<PieceOfData> > getValues(const std::vector< PieceOfData > & keys);
        virtual void saveValue(const KeyValuePair & item);
    };//class header

}//namespace
#endif // DATASRCBERKELEYDB_H

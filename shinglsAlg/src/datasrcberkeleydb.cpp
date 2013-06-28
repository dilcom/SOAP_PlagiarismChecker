#include "../headers/datasrcberkeleydb.h"
using namespace DePlaguarism;

DataSrcBerkeleyDB::DataSrcBerkeleyDB(const char * dbName, const char * envName, DBTYPE dbtype, u_int32_t flag)
    :env(new DbEnv(0)), master(true), tp(dbtype)
{
    try{
        MAKE_DIR(envName);
        env->open(envName, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD, 0);
        dbSrc = new Db(env, 0);
        dbSrc->set_flags(flag);
        dbSrc->open(0, dbName, NULL, dbtype, DB_CREATE | DB_THREAD, 0644);
    }
    catch (...){
        ///< TODO exception catching
    }
}

DataSrcBerkeleyDB::DataSrcBerkeleyDB(const char * dbName, DataSrcAbstract * mas, DBTYPE dbtype, u_int32_t flag)
    :master(false), tp(dbtype)
{
    try{
        DataSrcBerkeleyDB * localMaster = static_cast<DataSrcBerkeleyDB*> (mas);
        env = localMaster->getEnv();
        dbSrc = new Db(env, 0);
        dbSrc->set_flags(flag);
        dbSrc->open(0, dbName, NULL, dbtype, DB_CREATE | DB_THREAD, 0644);
    }
    catch (...){
        ///< TODO exception catching
    }
}

DataSrcBerkeleyDB::~DataSrcBerkeleyDB(){
    dbSrc->close(0);
    delete dbSrc;
    if (master) {
        env->close(0);
        delete env;
    }
}

std::vector<PieceOfData> * DataSrcBerkeleyDB::getValues(const PieceOfData & ikey){
    Dbc *cursorp;
    Dbt key(ikey.getValue(), ikey.getSize() );
    Dbt dataItem(0, 0);
    auto res = new std::vector<PieceOfData>;
    dbSrc->cursor(NULL, &cursorp, 0);
    int ret = cursorp->get(&key, &dataItem, DB_SET);
    while (ret != DB_NOTFOUND) {
        PieceOfData pieceOfResult((char*)(dataItem.get_data()), dataItem.get_size());
        res->push_back(pieceOfResult);
        ret = cursorp->get(&key, &dataItem, DB_NEXT_DUP);
    }
    cursorp->close();
    return res;
}

std::vector<PieceOfData> *DataSrcBerkeleyDB::getValues(const std::vector< PieceOfData > & keys){
    Dbc *cursorp;
    auto res = new std::vector<PieceOfData>;
    dbSrc->cursor(NULL, &cursorp, 0);
    Dbt dataItem(0, 0);
    for (auto it = keys.begin(); it != keys.end(); it++){
        Dbt key((*it).getValue(), (*it).getSize() );
        int ret = cursorp->get(&key, &dataItem, DB_SET);
        while (ret != DB_NOTFOUND) {
            PieceOfData pieceOfResult((char*)(dataItem.get_data()), dataItem.get_size());
            res->push_back(pieceOfResult);
            ret = cursorp->get(&key, &dataItem, DB_NEXT_DUP);
        }
    }
    cursorp->close();
    return res;
}

void DataSrcBerkeleyDB::saveValue(PieceOfData *ikey, PieceOfData *idata){
    Dbt key(ikey->getValue(), ikey->getSize());
    Dbt data(idata->getValue(), idata->getSize());
    Dbc * cursorp;
    dbSrc->cursor(NULL, &cursorp, DB_WRITECURSOR);
    cursorp->put(&key, &data, DB_KEYFIRST);
    cursorp->close();
}

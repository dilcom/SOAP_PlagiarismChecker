#include "../headers/datasrcberkeleydb.h"
using namespace Deplaguarism;

DataSrcBerkeleyDB::DataSrcBerkeleyDB(char * dbName, char * envName, DBTYPE dbtype, u_int32_t flag)
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

DataSrcBerkeleyDB::DataSrcBerkeleyDB(char * dbName, Db * mas, DBTYPE dbtype, u_int32_t flag)
    :env(mas->get_env()), master(false), dbSrc(new Db(env, 0)), tp(dbtype)
{
    try{
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

std::shared_ptr< std::vector<PieceOfData> > DataSrcBerkeleyDB::getValues(const PieceOfData & ikey){
    Dbc *cursorp;
    Dbt key(ikey.value, ikey.size );
    Dbt dataItem(0, 0);
    auto res = std::make_shared< std::vector<PieceOfData> >();
    dbSrc->cursor(NULL, &cursorp, 0);
    int ret = cursorp->get(&key, &dataItem, tp);
    while (ret != DB_NOTFOUND) {
        PieceOfData pieceOfResult((char*)(dataItem.get_data()), dataItem.get_size());
        res->push_back(pieceOfResult);
        ret = cursorp->get(&key, &dataItem, DB_NEXT_DUP);
    }
    cursorp->close();
    return res;
}

std::shared_ptr< std::vector<PieceOfData> > DataSrcBerkeleyDB::getValues(const std::vector< PieceOfData > & keys){
    Dbc *cursorp;
    auto res = std::make_shared< std::vector<PieceOfData> >();
    dbSrc->cursor(NULL, &cursorp, 0);
    Dbt dataItem(0, 0);
    for (auto it = keys.begin(); it != keys.end(); it++){
        Dbt key((*it).value, (*it).size );
        int ret = cursorp->get(&key, &dataItem, tp);
        while (ret != DB_NOTFOUND) {
            PieceOfData pieceOfResult((char*)(dataItem.get_data()), dataItem.get_size());
            res->push_back(pieceOfResult);
            ret = cursorp->get(&key, &dataItem, DB_NEXT_DUP);
        }
    }
    cursorp->close();
    return res;
}

void DataSrcBerkeleyDB::saveValue(const KeyValuePair & item){
    Dbt key(item.key->value, item.key->size);
    Dbt data(item.data->value, item.data->size);
    Dbc * cursorp;
    dbSrc->cursor(NULL, &cursorp, DB_WRITECURSOR);
    cursorp->put(&key, &data, DB_KEYFIRST);
    cursorp->close();
}

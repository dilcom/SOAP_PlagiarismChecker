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


std::vector<unsigned int> * DataSrcBerkeleyDB::getIdsByHashes(const unsigned int * hashes, unsigned int count){
    Dbc *cursorp;
    auto res = new std::vector<unsigned int>;
    size_t size = sizeof(hashes[0]);
    for (int i = 0; i < count; i += 1){
        Dbt key((void*)(hashes + i), size);
        Dbt dataItem(0, 0);
        dbSrc->cursor(NULL, &cursorp, 0);
        int ret = cursorp->get(&key, &dataItem, DB_SET);
        while (ret != DB_NOTFOUND) {
            res->push_back( * ( (unsigned int *) (dataItem.get_data()) ) );
            ret = cursorp->get(&key, &dataItem, DB_NEXT_DUP);
        }
    }
    cursorp->close();
    return res;
}

void DataSrcBerkeleyDB::saveIds(unsigned int docNumber, const unsigned int * hashes, unsigned int count){
    Dbc * cursorp;
    size_t size = sizeof(hashes[0]);
    Dbt key((void*)hashes, size);
    Dbt data((void*)&docNumber, sizeof(docNumber));
    for (int i = 0; i < count; i += 1){
        key.set_data((void*)(hashes + i));
        key.set_size(size);
        dbSrc->cursor(NULL, &cursorp, DB_WRITECURSOR);
        cursorp->put(&key, &data, DB_KEYFIRST);
    }
    cursorp->close();
}

void DataSrcBerkeleyDB::saveDocument(DocHeader header, t__text * txt){
    int length = sizeof(header) + header.authorGroup_len + header.authorName_len + header.data_len
        + header.textName_len;
    char * textDocData = new char[length];
    char * pointer = textDocData;
    memcpy((void*)pointer, &(header), sizeof(DocHeader));
    pointer += sizeof(DocHeader);
    memcpy(pointer, txt->authorGroup, header.authorGroup_len);
    pointer += header.authorGroup_len;
    memcpy(pointer, txt->authorName, header.authorName_len);
    pointer += header.authorName_len;
    memcpy(pointer, txt->streamData, header.data_len);
    pointer += header.data_len;
    memcpy(pointer, txt->name, header.textName_len);
    pointer += header.textName_len;
    Dbc * cursorp;
    Dbt key((void*)&(header.number), sizeof(header.number));
    Dbt data(textDocData, length);
    dbSrc->cursor(NULL, &cursorp, DB_WRITECURSOR);
    cursorp->put(&key, &data, DB_KEYFIRST);
    cursorp->close();
    delete[] textDocData;
}

void DataSrcBerkeleyDB::getDocument(unsigned int docNumber, t__text * trgt, soap * parent){
    Dbc *cursorp;
    dbSrc->cursor(NULL, &cursorp, 0);
    Dbt dataItem(0, 0);
    Dbt key((void*)(&docNumber), sizeof(docNumber) );
    cursorp->get(&key, &dataItem, DB_SET);
    char * pointer = (char*)(dataItem.get_data());
    DocHeader header = *(DocHeader *) pointer;
    trgt->type = header.type;
    pointer += sizeof(DocHeader);

    trgt->authorGroup = reinterpret_cast<char*>(soap_malloc(parent, header.authorGroup_len + 1));
    memcpy(trgt->authorGroup, pointer, header.authorGroup_len);
    trgt->authorGroup[header.authorGroup_len] = '\0';
    pointer += header.authorGroup_len;

    trgt->authorName = reinterpret_cast<char*>(soap_malloc(parent, header.authorName_len + 1));
    memcpy(trgt->authorName, pointer, header.authorName_len);
    trgt->authorName[header.authorName_len] = '\0';
    pointer += header.authorName_len;

    trgt->streamData = reinterpret_cast<char*>(soap_malloc(parent, header.data_len + 1));
    memcpy(trgt->streamData, pointer, header.data_len);
    trgt->streamData[header.data_len] = '\0';
    pointer += header.data_len;

    trgt->name = reinterpret_cast<char*>(soap_malloc(parent, header.textName_len + 1));
    memcpy(trgt->name, pointer, header.textName_len);;
    trgt->name[header.textName_len] = '\0';
    pointer += header.textName_len;

    char * date = asctime(&(header.dateTime));
    trgt->date = reinterpret_cast<char*>(soap_malloc(parent, strlen(date) + 1));
    strcpy(trgt->date, date);
    cursorp->close();
}

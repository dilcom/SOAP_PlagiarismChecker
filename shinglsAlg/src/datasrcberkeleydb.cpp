#include "../headers/datasrcberkeleydb.h"
using namespace DePlaguarism;

DbEnv * DePlaguarism::DataSrcBerkeleyDB::env;
Db * DePlaguarism::DataSrcBerkeleyDB::dbSrcHashes;
Db * DePlaguarism::DataSrcBerkeleyDB::dbSrcDocs;

DataSrcBerkeleyDB::DataSrcBerkeleyDB(const char * envName, const char *hashDbName, const char *docsDbName, bool mainFlag)
{
    mainClient = mainFlag;
    if (mainFlag){
        try{
            MAKE_DIR(envName);
            env = new DbEnv(0);
            env->open(envName, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD, 0);
            dbSrcHashes = new Db(env, 0);
            dbSrcHashes->set_flags(DB_DUP);
            dbSrcHashes->open(0, hashDbName, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0644);
            dbSrcDocs = new Db(env, 0);
            dbSrcDocs->open(0, docsDbName, NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
        }
        catch (...){
            //error->append("Database opening error!");
        }
    }
}

DataSrcBerkeleyDB::~DataSrcBerkeleyDB(){
    if (mainClient){
        dbSrcHashes->close(0);
        delete dbSrcHashes;
        dbSrcDocs->close(0);
        delete dbSrcDocs;
        env->close(0);
        delete env;
    }
    //delete error;
}


std::vector<unsigned int> * DataSrcBerkeleyDB::getIdsByHashes(const unsigned int * hashes, unsigned int count){
    Dbc *cursorp;
    auto res = new std::vector<unsigned int>;
    size_t size = sizeof(hashes[0]);
    for (int i = 0; i < count; i += 1){
        Dbt key((void*)(hashes + i), size);
        Dbt dataItem(0, 0);
        dbSrcHashes->cursor(NULL, &cursorp, 0);
        int ret = cursorp->get(&key, &dataItem, DB_SET);
        while (ret != DB_NOTFOUND) {
            res->push_back( * ( (unsigned int *) (dataItem.get_data()) ) );
            ret = cursorp->get(&key, &dataItem, DB_NEXT_DUP);
        }
        cursorp->close();
    }
    return res;
}

void DataSrcBerkeleyDB::saveIds(unsigned int docNumber, const unsigned int * hashes, unsigned int count){
    Dbc * cursorp;
    size_t size = sizeof(hashes[0]);
    Dbt data((void*)(&docNumber), sizeof(docNumber));
    for (int i = 0; i < count; i += 1){
        Dbt key((void*)(hashes + i), size);
        dbSrcHashes->cursor(NULL, &cursorp, DB_WRITECURSOR);
        cursorp->put(&key, &data, DB_KEYFIRST);
        cursorp->close();
    }
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
    dbSrcDocs->cursor(NULL, &cursorp, DB_WRITECURSOR);
    cursorp->put(&key, &data, DB_KEYFIRST);
    cursorp->close();
    delete[] textDocData;
}

void DataSrcBerkeleyDB::getDocument(unsigned int docNumber, t__text * trgt, soap * parent){
    Dbc *cursorp;
    dbSrcDocs->cursor(NULL, &cursorp, 0);
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

#include "../headers/datasrcberkeleydb.h"
using namespace DePlaguarism;

DbEnv * DePlaguarism::DataSrcBerkeleyDB::env;
Db * DePlaguarism::DataSrcBerkeleyDB::dbSrcHashes;
Db * DePlaguarism::DataSrcBerkeleyDB::dbSrcDocs;
unsigned int DePlaguarism::DataSrcBerkeleyDB::docNumber;
MUTEX_TYPE DePlaguarism::DataSrcBerkeleyDB::mtx;

DataSrcBerkeleyDB::DataSrcBerkeleyDB(const char * envName, const char *hashDbName, const char *docsDbName, bool mainFlag)
{
    mainClient = mainFlag;
    if (mainFlag){
        try{
            MAKE_DIR(envName);
            MUTEX_SETUP(mtx);
            env = new DbEnv(0);
            env->open(envName, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD, 0);
            dbSrcHashes = new Db(env, 0);
            dbSrcHashes->set_flags(DB_DUP);
            dbSrcHashes->open(0, hashDbName, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0644);
            dbSrcDocs = new Db(env, 0);
            dbSrcDocs->open(0, docsDbName, NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
            docNumber = 1;
            Dbt dataDocNum(NULL, 0);
            unsigned int key = 0;
            Dbt keyDocNum((void*)(&key), sizeof(key));
            Dbc *cursorp;
            dbSrcDocs->cursor(NULL, &cursorp, 0);
            int ret = cursorp->get(&keyDocNum, &dataDocNum, DB_SET);
            if (ret != DB_NOTFOUND)
                docNumber = *((unsigned int *)dataDocNum.get_data());
            cursorp->close();
        }
        catch (...){
        }
    }
}

DataSrcBerkeleyDB::~DataSrcBerkeleyDB(){
    if (mainClient){
        saveDocNumber();
        dbSrcHashes->close(0);
        delete dbSrcHashes;
        dbSrcDocs->close(0);
        delete dbSrcDocs;
        env->close(0);
        delete env;
        MUTEX_CLEANUP(mtx);
    }
}


std::vector<unsigned int> * DataSrcBerkeleyDB::getIdsByHashes(const unsigned int * hashes, unsigned int count){
    Dbc *cursorp;
    auto res = new std::vector<unsigned int>;
    size_t size = sizeof(hashes[0]);
    for (unsigned int i = 0; i < count; i += 1){
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

void DataSrcBerkeleyDB::save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt){
    ///< 0. we need new document number
    MUTEX_LOCK(mtx);
    unsigned int localDocNumber = docNumber;
    docNumber += 1;
    saveDocNumber();
    MUTEX_UNLOCK(mtx);
    //TO DO document number!
    ///< 1. hash->localDocNumber
    Dbc * cursorp;
    size_t size = sizeof(hashes[0]);
    Dbt data((void*)(&localDocNumber), sizeof(localDocNumber));
    for (unsigned int i = 0; i < count; i += 1){
        Dbt key((void*)(hashes + i), size);
        dbSrcHashes->cursor(NULL, &cursorp, DB_WRITECURSOR);
        cursorp->put(&key, &data, DB_KEYFIRST);
        cursorp->close();
    }
    ///< 2. localDocNumber->document
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
    Dbc * cursorq;
    Dbt keyDoc((void*)&(localDocNumber), sizeof(localDocNumber));
    Dbt dataDoc(textDocData, length);
    dbSrcDocs->cursor(NULL, &cursorq, DB_WRITECURSOR);
    cursorq->put(&keyDoc, &dataDoc, DB_KEYFIRST);
    cursorq->close();
    delete[] textDocData;
}

void DataSrcBerkeleyDB::saveDocNumber(){
    unsigned int key = 0;
    Dbt dataDocNum(&docNumber, sizeof(docNumber));
    Dbt keyDocNum((void*)(&key), sizeof(key));
    Dbc *cursorp;
    dbSrcDocs->cursor(NULL, &cursorp, DB_WRITECURSOR);
    cursorp->put(&keyDocNum, &dataDocNum, DB_KEYFIRST);
    cursorp->close();
    std::cout << docNumber << "\n";
}

void DataSrcBerkeleyDB::getDocument(unsigned int docNumber, t__text ** trgtPtr, soap * parent){
    Dbc *cursorp;
    t__text * trgt = *trgtPtr;
    trgt->creator = parent;
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
    memcpy(trgt->name, pointer, header.textName_len);
    trgt->name[header.textName_len] = '\0';
    pointer += header.textName_len;

    char * date = asctime(&(header.dateTime));
    size_t dateLen = strlen(date);
    trgt->date = reinterpret_cast<char*>(soap_malloc(parent, dateLen + 1));
    memcpy(trgt->date, date, dateLen);
    trgt->date[dateLen] = '\0';
    cursorp->close();
}

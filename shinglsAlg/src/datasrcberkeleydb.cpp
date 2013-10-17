#include "../headers/datasrcberkeleydb.h"
using namespace DePlagiarism;
using namespace std;

DbEnv * DePlagiarism::DataSrcBerkeleyDB::m_env;
Db * DePlagiarism::DataSrcBerkeleyDB::m_dbSrcHashes;
Db * DePlagiarism::DataSrcBerkeleyDB::m_dbSrcDocs;
unsigned int DePlagiarism::DataSrcBerkeleyDB::m_docNumber;
MUTEX_TYPE DePlagiarism::DataSrcBerkeleyDB::m_mtx;

typedef vector<string>::const_iterator vecStrConstIter;

DataSrcBerkeleyDB::DataSrcBerkeleyDB(const char * envName, const char *hashDbName, const char *docsDbName, bool mainFlag, char *threadName)
{
    m_mainClient = mainFlag;
    if (mainFlag){
        try{
            MAKE_DIR(envName);
            MUTEX_SETUP(m_mtx);
            m_env = new DbEnv(0);
            m_env->open(envName, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD, 0);
            m_dbSrcHashes = new Db(m_env, 0);
            m_dbSrcHashes->set_flags(DB_DUP);
            m_dbSrcHashes->open(0, hashDbName, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0644);
            m_dbSrcDocs = new Db(m_env, 0);
            m_dbSrcDocs->open(0, docsDbName, NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
            m_docNumber = 1;
            Dbt dataDocNum(NULL, 0);
            unsigned int key = 0;
            Dbt keyDocNum((void*)(&key), sizeof(key));
            Dbc *cursorp;
            m_dbSrcDocs->cursor(NULL, &cursorp, 0);
            int ret = cursorp->get(&keyDocNum, &dataDocNum, DB_SET);
            if (ret != DB_NOTFOUND)
                m_docNumber = *((unsigned int *)dataDocNum.get_data());
            cursorp->close();
        }
        catch (...){
        }
    }
    m_threadName = threadName;
    m_logger = Log::getLogger();
}

DataSrcBerkeleyDB::~DataSrcBerkeleyDB(){
    if (m_mainClient){
        saveDocNumber();
        m_dbSrcHashes->close(0);
        delete m_dbSrcHashes;
        m_dbSrcDocs->close(0);
        delete m_dbSrcDocs;
        m_env->close(0);
        delete m_env;
        MUTEX_CLEANUP(m_mtx);
    }
}


getIdsByHashesResult__t * DataSrcBerkeleyDB::getIdsByHashes(const unsigned int * hashes, unsigned int count){
    Dbc *cursorp;
    getIdsByHashesResult__t * res = new getIdsByHashesResult__t;
    size_t size = sizeof(hashes[0]);
    for (unsigned int i = 0; i < count; i += 1){
        Dbt key((void*)(hashes + i), size);
        Dbt dataItem(0, 0);
        m_dbSrcHashes->cursor(NULL, &cursorp, 0);
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
    // 0. we need new document number
    MUTEX_LOCK(m_mtx);
    unsigned int localDocNumber = m_docNumber;
    m_docNumber += 1;
    saveDocNumber();
    MUTEX_UNLOCK(m_mtx);
    //TO DO document number!
    // 1. hash->localDocNumber
    Dbc * cursorp;
    size_t size = sizeof(hashes[0]);
    Dbt data((void*)(&localDocNumber), sizeof(localDocNumber));
    for (unsigned int i = 0; i < count; i += 1){
        Dbt key((void*)(hashes + i), size);
        m_dbSrcHashes->cursor(NULL, &cursorp, DB_WRITECURSOR);
        cursorp->put(&key, &data, DB_KEYFIRST);
        cursorp->close();
    }
    // 2. localDocNumber->document
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
    m_dbSrcDocs->cursor(NULL, &cursorq, DB_WRITECURSOR);
    cursorq->put(&keyDoc, &dataDoc, DB_KEYFIRST);
    cursorq->close();
    delete[] textDocData;
}

void DataSrcBerkeleyDB::saveDocNumber(){
    unsigned int key = 0;
    Dbt dataDocNum(&m_docNumber, sizeof(m_docNumber));
    Dbt keyDocNum((void*)(&key), sizeof(key));
    Dbc *cursorp;
    m_dbSrcDocs->cursor(NULL, &cursorp, DB_WRITECURSOR);
    cursorp->put(&keyDocNum, &dataDocNum, DB_KEYFIRST);
    cursorp->close();
    cout << m_docNumber << "\n";
}

void DataSrcBerkeleyDB::getDocument(unsigned int docNumber, t__text ** trgtPtr, soap * parent){
    Dbc *cursorp;
    t__text * trgt = *trgtPtr;
    trgt->creator = parent;
    m_dbSrcDocs->cursor(NULL, &cursorp, 0);
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

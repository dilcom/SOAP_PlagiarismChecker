#include "../headers/datasrcrediscluster.h"
#include <sds.h>
using namespace DePlagiarism;
using namespace std;

typedef vector<string>::const_iterator vecStrConstIter;
typedef vector<DataSrcRedisCluster::Task::BasicQuerry*>::iterator vecTasksIter;
typedef vector<redisReply*>::const_iterator vecReplyConstIter;

uint16_t DePlagiarism::crc16(const char *buf, int len);

DataSrcRedisCluster::Task::Task(): m_ready(false), m_error(false) {
    MUTEX_SETUP(m_mtx);
    COND_SETUP(m_cond);
}

DataSrcRedisCluster::Task::~Task() {
    MUTEX_CLEANUP(m_mtx);
    COND_CLEANUP(m_cond);
    clear();
}

void DataSrcRedisCluster::Task::addQuerry(redisContext * cont, bool needResult, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    BasicQuerry * quer = new BasicQuerry(cont, needResult, format, ap);
    va_end(ap);
    m_querries.push_back(quer);
}

void DataSrcRedisCluster::Task::addResult(redisReply * rep) {
    m_result.push_back(rep);
}

void DataSrcRedisCluster::Task::signalReady(bool error) {
    MUTEX_LOCK(m_mtx);
    m_error = error;
    m_ready = true;
    COND_SIGNAL(m_cond);
    MUTEX_UNLOCK(m_mtx);
}

bool DataSrcRedisCluster::Task::waitForResult() {
    MUTEX_LOCK(m_mtx);
    while (!m_ready)
        COND_WAIT(m_cond, m_mtx);
    MUTEX_UNLOCK(m_mtx);
    return !m_error;
}

void DataSrcRedisCluster::Task::clearResult() {
    while ( !m_result.empty() ) {
        freeReplyObject( m_result.back() );
        m_result.pop_back();
    }
}

void DataSrcRedisCluster::Task::clearTasks() {
    while ( !m_querries.empty() ) {
        delete m_querries.back();
        m_querries.pop_back();
    }
}

void DataSrcRedisCluster::Task::clear() {
    clearResult();
    clearTasks();
}

const std::vector<redisReply*> * DataSrcRedisCluster::Task::getResult() {
    return &m_result;
}

vector<string> * DePlagiarism::split(const string &s, char delim, vector<string> *elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems->push_back(item);
    }
    return elems;
}

void * DePlagiarism::processRedisQuerries(void * client) {
    DataSrcRedisCluster * localClient = static_cast<DataSrcRedisCluster*>(client);
    DataSrcRedisCluster::Task *localTask;
    while ( (localTask = localClient->m_dbQueue.pop()) != NULL) {
        bool flag = true; //true if we should signal at the end        
        for (vecTasksIter i = localTask->m_querries.begin(); i != localTask->m_querries.end(); i++) {
            DataSrcRedisCluster::Task::BasicQuerry * j = *i;
            redisReply * rep;
            try {
                rep = localClient->getRedisReply(j->m_cont, j->m_command, j->m_len);
            } catch (...) {
                try {
                    localClient->reinitializeCluster();
                } catch (const char * str) {
                    localClient->m_logger->error("{%s}: Initialize cluster failed: '%s'. Sleep 5 seconds before next task", localClient->m_threadName, str);
                    SLEEP(5000);
                }
                localTask->signalReady(true);
                break;
                flag = false;
            }
            if (j->m_needResult)
                localTask->addResult(rep);
            else
                freeReplyObject(rep);
        }
        if (flag)
            localTask->signalReady(false);
    }
}

DataSrcRedisCluster::DataSrcRedisCluster(const char * ipAddress, int port, const char *threadName)
    :m_clientsCount(0), m_dbQueue(DefaultValues::MAX_QUEUE_REDIS)
{
    m_mainClientIp = new char[64];
    strcpy(m_mainClientIp, ipAddress);
    m_mainClientPort = port;
    redisReply * clusterInfo;
    try {
        m_mainClient = redisConnect(m_mainClientIp, m_mainClientPort);
        // did we connect?
        clusterInfo = redisCommandWithReply(m_mainClient, "cluster nodes");
        m_lastMapping = crc16(clusterInfo->str, clusterInfo->len);
        string conf(clusterInfo->str);
        initializeCluster(&conf);
    }
    catch(...) {
        redisFree(m_mainClient);
        throw("Cluster init error!");
    }
    freeReplyObject(clusterInfo);
    size_t len = strlen(threadName) + 2;
    m_threadName = new char[len];
    memcpy(m_threadName, threadName, len);
    m_logger = Log::getLogger();
    int threadID;
    m_logger->notice("{%s}: Starting DB worker thread...", m_threadName);    
    THREAD_CREATE(&m_clientThread, processRedisQuerries, (void*)this, &threadID);
    m_logger->notice("{%s}: ...started!", m_threadName);
}

void DataSrcRedisCluster::initializeCluster(string * configString){
    vector<string> confStrs;
    split(*configString, '\n', &confStrs);
    // each of vector elements now represents one node in redis cluster
    // some of them may be slaves so we`ll dec clients count each time we meet it
    m_clientsCount = confStrs.size();
    m_clients = new redisContext*[m_clientsCount];
    for (int i = 1; i < m_clientsCount; i += 1)
        m_clients[i] = NULL;
    m_clients[0] = m_mainClient;
    // main clients is always num 0
    int currentNodeNumber = 1;
    int nodeId;
    for (vecStrConstIter i = confStrs.begin(); i != confStrs.end(); i += 1){
        vector<string> confTokens;
        split(*i, ' ', &confTokens);
        /* confTokens[0] - node id
         * confTokens[1] - ip:port
         * confTokens[2] - flags (we`re interested in master\slave flag only)
         * confTokens[3] - master_name\-
         * confTokens[4] - ping send
         * confTokens[5] - pong recieved
         * confTokens[6] - connected\disconnected
         * confTokens[7-...] - slots assigned
         */
        if (confTokens[2].find("master") != string::npos){
            try {
                confTokens.at(7);
                // if there is no slots for this node in config, it will throw the exception
            }
            catch(const out_of_range& oor) {
                // it`s slave! we should process next line
                if (confTokens[2].find("myself") == string::npos)
                    m_clientsCount -= 1;
                continue;
            }
            if (confTokens[6] == "disconnected")
                throw("One of master nodes is down!");
            // 1. connecting only if it`s not myself
            if (confTokens[2].find("myself") == string::npos){
                char ip[64];
                unsigned int pos = confTokens[1].find(":");
                for (size_t j = 0; j < pos; j += 1)
                    ip[j] = confTokens[1][j];
                ip[pos] = '\0';
                string a = confTokens[1].substr(pos + 1);
                int port;
                istringstream ( a ) >> port;
                m_clients[currentNodeNumber] = redisConnect(ip, port);
                nodeId = currentNodeNumber;
                currentNodeNumber += 1;
            } else {
                nodeId = 0;
            }
            // 2. slots assign we do it once every time cluster changes
            vecStrConstIter slotIterator = confTokens.begin();
            slotIterator += 7;
            while (slotIterator != confTokens.end()){
                const string * slotStr = &(*slotIterator);
                if (slotStr[0][0] != '[') {
                    if (slotStr->find("-") != string::npos){ // slot range or one slot?
                        int slotRange[2];
                        istringstream ( *slotStr ) >> slotRange[0] >> slotRange[1];
                        slotRange[1] = - slotRange[1];
                        for (int j = slotRange[0]; j <= slotRange[1]; j += 1)
                            slotMap[j] = nodeId;
                    } else {
                        int slot;
                        istringstream ( *slotStr ) >> slot;
                        slotMap[slot] = nodeId;
                    }
                }
                slotIterator += 1;
            }
        } else {
            if (confTokens[2].find("myself") == string::npos)
                m_clientsCount -= 1;
            // decrement clientsCount because we are faced with slave node and it`s not clients[0]
        }
    }
}

void DataSrcRedisCluster::reinitializeCluster() {
    for (int i = 0; i < m_clientsCount; i += 1){
        if (m_clients[i] != NULL && m_clients[i]->err == REDIS_OK){
            redisReply * rep = (redisReply *)redisCommand(m_clients[i], "exec"); // just to be sure we`re outside of transaction
            if (rep != NULL)
                freeReplyObject(rep);
        }
    }
    bool clusterNotAvalible = false;
    redisReply * clusterInfo;
    do {
        clusterInfo = NULL;
        int i = 0;
        if (m_mainClient != NULL && m_mainClient->err == REDIS_OK)
            clusterInfo = (redisReply *)(redisCommand(m_mainClient, "cluster nodes"));
        if (clusterInfo == NULL || (clusterInfo->type == REDIS_REPLY_ERROR || clusterInfo->type == REDIS_REPLY_NIL)){
            i += 1;
            bool flag = true;
            while (i < m_clientsCount && flag){
                if (clusterInfo != NULL)
                    freeReplyObject(clusterInfo);
                if (m_clients[i] == NULL || m_clients[i]->err != REDIS_OK){
                    i += 1;
                    continue;
                }
                clusterInfo = (redisReply *)(redisCommand(m_clients[i], "cluster nodes"));
                flag = !clusterInfo || (clusterInfo->type == REDIS_REPLY_ERROR);
                i += 1;
            }
            // now i is a number of working node or all the nodes down (i = clientsCount)
            if (flag){
                if (clusterInfo)
                    freeReplyObject(clusterInfo);
                // lets try to reconnect to mainClient again
                clusterInfo = NULL;
                redisContext * tmp = NULL;
                try {
                    tmp = redisConnect(m_mainClientIp, m_mainClientPort);
                    clusterInfo = redisCommandWithReply(tmp, "cluster nodes");
                    i = 0;
                }
                catch (...) {
                    if (tmp != NULL)
                        redisFree(tmp);
                    if (clusterInfo != NULL)
                        freeReplyObject(clusterInfo);
                    throw("No one node responds!");
                }

                if (m_clients[0] != NULL)
                    redisFree(m_clients[0]);
                m_clients[0] = tmp;
            }
        }
        string s(clusterInfo->str);
        clusterNotAvalible = false;
        // now lets try to reconnect (saving connection to old cluster)
        redisContext ** oldClients = m_clients;
        int oldCliCount = m_clientsCount;
        m_mainClient = oldClients[i]; // first working client
        try{
            initializeCluster(&s);
            // if we successed lets free previous cluster
            for (int j = 0; j < oldCliCount; j += 1){
                if (j != i)
                    redisFree(oldClients[j]);
            }
            delete[] oldClients;
        }
        catch (const char * str) {
            // returning to previous state if we failed
            m_logger->error("{%s}: Initialize cluster failed: '%s'. Waiting 5 seconds before next try", m_threadName, str);
            deinitializeCluster(); // deletes all the new nodes except mainClient
            m_mainClient = oldClients[0];
            m_clients = oldClients;
            m_clientsCount = oldCliCount;
            clusterNotAvalible = true;
            freeReplyObject(clusterInfo);
            SLEEP(1000);
        }
    } while (clusterNotAvalible);
    freeReplyObject(clusterInfo);
}

void DataSrcRedisCluster::deinitializeCluster(){
    // does not deinit main client!!!
    for (int i = 1; i < m_clientsCount; i += 1){
        if (m_clients[i] != NULL)
            redisFree(m_clients[i]);
    }
    delete[] m_clients;
}

void DataSrcRedisCluster::redisCommandWithoutReply(redisContext *c, const char *format, ...) {
    if (c == NULL || c->err != REDIS_OK)
        throw("Something happend with redis context!");
    va_list ap;
    void *rep = NULL;
    va_start(ap,format);
    rep = redisvCommand(c,format,ap);
    va_end(ap);
    if (rep == NULL)
        throw("Reply is NULL pointer!");
    redisReply * reply = (redisReply*)rep;
    if (reply->type == REDIS_REPLY_ERROR){
        freeReplyObject(reply);
        throw("Redis got an error in reply!");
    }
    freeReplyObject(reply);
}

redisReply * DataSrcRedisCluster::redisCommandWithReply(redisContext *c, const char *format, ...) {
    if (c == NULL || c->err != REDIS_OK)
        throw("Something happend with redis context!");
    va_list ap;
    void *rep = NULL;
    va_start(ap,format);
    rep = redisvCommand(c,format,ap);
    va_end(ap);
    if (rep == NULL)
        throw("Reply is NULL pointer!");
    redisReply * reply = (redisReply*)rep;
    if (reply->type == REDIS_REPLY_ERROR){
        freeReplyObject(reply);
        throw("Redis got an error in reply!");
    }
    for (unsigned int i = 0; i < reply->elements; i += 1){
        if (reply->element[i]->type == REDIS_REPLY_ERROR){
            freeReplyObject(reply);
            throw("Redis got an error in reply!");
        }
    }
    return reply;
}

redisReply * DataSrcRedisCluster::getRedisReply(redisContext *c, char *cmd, size_t len) {
    if (c == NULL || c->err != REDIS_OK)
        throw("Something happend with redis context!");    
    sds newbuf = sdscatlen(c->obuf,(const void *)cmd,len);
    if (newbuf == NULL) {
        c->err = REDIS_ERR_OOM;
        strcpy(c->errstr, "Out of memory");
        throw (REDIS_ERR);
    }
    c->obuf = newbuf;
    void *voidReply;    
    if (c->flags & REDIS_BLOCK) {
        if (redisGetReply(c,&voidReply) != REDIS_OK)
            throw (REDIS_ERR);
    } else {
        throw (REDIS_ERR);
    }
    redisReply * reply = (redisReply*)voidReply;
    if (reply->type == REDIS_REPLY_ERROR){
        freeReplyObject(reply);
        throw("Redis got an error in reply!");
    }
    for (unsigned int i = 0; i < reply->elements; i += 1){
        if (reply->element[i]->type == REDIS_REPLY_ERROR){
            freeReplyObject(reply);
            throw("Redis got an error in reply!");
        }
    }
    return reply;
}

void DataSrcRedisCluster::save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt){
    // 0. we need new document number
    unsigned int docNumber;
    bool flag = false;
    do {
        flag = false;
        Task docNumberTask;
        docNumberTask.addQuerry(m_clients[slotMap[crc16("current:document:number", 23) & 0x3fff]], true, "incr current:document:number");
        while ( !m_dbQueue.push(&docNumberTask) )
            SLEEP(100); // sleep if queue is full   
        if ( docNumberTask.waitForResult() ) {
            docNumber = docNumberTask.getResult()->back()->integer;
        } else {
            flag = true;
        }
    } while (flag);
    do {
        flag = false;
        // 1. start a transaction
        Task localTask;
        for (int i = 0; i < m_clientsCount; i += 1){
            localTask.addQuerry(m_clients[i], false, "multi");
        }
        // 2. add all the data to save
        // 2.1. hash->docNumber
        for (unsigned int i = 0; i < count; i += 1){
            char tmp[20];
            sprintf(tmp, "hash:%d", hashes[i]);
            int len = strlen(tmp);
            redisContext * context = m_clients[slotMap[crc16(tmp, len) & 0x3fff]];
            localTask.addQuerry(context, false, "sadd %b %d", tmp, len, docNumber);
        }
        // 2.2. docNumber->document
        char key[32];
        sprintf(key, "document:number:%u", docNumber);
        int lenDoc = strlen(key);
        redisContext * docContext = m_clients[slotMap[crc16(key, lenDoc) & 0x3fff]];
        localTask.addQuerry(docContext, false, "hset %b textName %b", key, (size_t) lenDoc, txt->name, header.textName_len); // textName is a name of field in hash
        localTask.addQuerry(docContext, false, "hset %b textData %b", key, (size_t) lenDoc, txt->streamData, header.data_len);
        localTask.addQuerry(docContext, false, "hset %b authorName %b", key, (size_t) lenDoc, txt->authorName, header.authorName_len);
        localTask.addQuerry(docContext, false, "hset %b authorGroup %b", key, (size_t) lenDoc, txt->authorGroup, header.authorGroup_len);
        localTask.addQuerry(docContext, false, "hset %b textType %b", key, (size_t) lenDoc, &(txt->type), sizeof(txt->type));
        char * date = asctime(&(header.dateTime));
        localTask.addQuerry(docContext, false, "hset %b date %b", key, (size_t) lenDoc, date, strlen(date));
        // 3. finish the transaction
        for (int i = 0; i < m_clientsCount; i += 1){
            localTask.addQuerry(m_clients[i], false, "exec");
        }   
        while ( !m_dbQueue.push(&localTask) )
            SLEEP(100); // sleep if queue is full         
        if ( !localTask.waitForResult() ) {
            flag = true;
        }
    } while (flag);
}


void DataSrcRedisCluster::getDocument(unsigned int docNumber, t__text **trgtPtr, soap * parent){
    char key[30];
    t__text * trgt = *trgtPtr;
    trgt->creator = parent;
    sprintf(key, "document:number:%d", docNumber);
    int len = strlen(key);
    int contNum = slotMap[crc16(key, len) & 0x3fff];
    redisContext * context = m_clients[contNum];
    bool flag = false;
    do {
        Task localTask;
        flag = false;
        localTask.addQuerry(context, true, "hget %b textName", key, (size_t) len);
        localTask.addQuerry(context, true, "hget %b textData", key, (size_t) len);
        localTask.addQuerry(context, true, "hget %b authorName", key, (size_t) len);
        localTask.addQuerry(context, true, "hget %b authorGroup", key, (size_t) len);
        localTask.addQuerry(context, true, "hget %b textType", key, (size_t) len);
        localTask.addQuerry(context, true, "hget %b date", key, (size_t) len);        
        while ( !m_dbQueue.push(&localTask) )
            SLEEP(100); // sleep if queue is full           
        if ( localTask.waitForResult() ) {
            vecReplyConstIter i= localTask.getResult()->begin();
            redisReply * rep = *i;
            if (rep->type == REDIS_REPLY_NIL) { //no such document in DB
                delete trgt;
                *trgtPtr = NULL;
                return;
            }            
            trgt->name = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
            strcpy(trgt->name, rep->str);
            rep = *(++i);
            trgt->streamData = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
            strcpy(trgt->streamData, rep->str);
            rep = *(++i);
            trgt->authorName = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
            strcpy(trgt->authorName, rep->str);
            rep = *(++i);
            trgt->authorGroup = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
            strcpy(trgt->authorGroup, rep->str);
            rep = *(++i);
            trgt->type = *((t__type*)(rep->str));
            rep = *(++i);
            trgt->date = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
            strcpy(trgt->date, rep->str);
        } else {
            flag = true;
        }
    } while (flag);
}

getIdsByHashesResult__t * DataSrcRedisCluster::getIdsByHashes(const unsigned int * hashes, unsigned int count){
    getIdsByHashesResult__t * result = new getIdsByHashesResult__t();
    bool flag = false;
    do {
        flag = false;
        result->clear();
        Task localTask;
        for (unsigned int i = 0; i < count; i += 1){
            char tmp[20];
            sprintf(tmp, "hash:%d", hashes[i]);
            int k = slotMap[crc16(tmp, strlen(tmp)) & 0x3fff];
            redisContext * client = m_clients[k];
            localTask.addQuerry(client, true, "smembers %s", tmp);
        }
        while ( !m_dbQueue.push(&localTask) )
            SLEEP(100); // sleep if queue is full        
        if ( localTask.waitForResult() ) { // wait for another thread to process task
            //returns true if everything OK and false if not
            const vector<redisReply*> * resArr = localTask.getResult();
            for (vecReplyConstIter i = resArr->begin(); i != resArr->end(); i++) {
                redisReply * rep = *i;
                int elems = rep->elements;
                for (unsigned int j = 0; j < elems; j += 1){
                    unsigned int tmpInt = 0;
                    char * tmpStr = rep->element[j]->str;
                    while (*tmpStr != '\0') {
                        tmpInt *= 10;
                        tmpInt += *tmpStr - '0';
                        tmpStr += 1;
                    }
                    result->push_back(tmpInt);
                }
            }
        } else {
            flag = true;
        }        
    } while (flag);
    return result;
}

DataSrcRedisCluster::~DataSrcRedisCluster(){
    m_logger->notice("{%s}: Waiting for dataClient thread to terminate... ", m_threadName);
    stop();
    m_logger->notice("{%s}: ...terminated", m_threadName);
    delete[] m_mainClientIp;
    delete[] m_threadName;
    deinitializeCluster();
    redisFree(m_mainClient);
}

void DataSrcRedisCluster::stop() {
    m_dbQueue.push(NULL);
    THREAD_JOIN(m_clientThread);
}


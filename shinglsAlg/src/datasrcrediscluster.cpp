#include "../headers/datasrcrediscluster.h"

using namespace DePlaguarism;

uint8_t DataSrcRedisCluster::slotMap[16384];
uint16_t DataSrcRedisCluster::lastMapping; ///< contains crc16 hash of last config
MUTEX_TYPE DataSrcRedisCluster::mtx;///< used in reinitializeCluster to prevent remapping
uint16_t DePlaguarism::crc16(const char *buf, int len);

std::vector<std::string> * DePlaguarism::split(const std::string &s, char delim, std::vector<std::string> *elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems->push_back(item);
    }
    return elems;
}


DataSrcRedisCluster::DataSrcRedisCluster(const char * ipAddress, int port)
    :clientsCount(0), error(new std::string(""))
{
    MUTEX_SETUP(mtx);
    mainClient = redisConnect(ipAddress, port);
    if (mainClient->err) {
        error->append("Can`t connect to redis! Errstring: '");
        error->append(mainClient->errstr);
        error->append("'.");
    } else {
        redisReply * clusterInfo = (redisReply *)(redisCommand(mainClient, "cluster nodes"));
        if (clusterInfo->type == REDIS_REPLY_ERROR) {
            error->append("Something wrong with cluster! Errstring: '");
            error->append(clusterInfo->str);
            error->append("'.");
        } else {
            lastMapping = crc16(clusterInfo->str, clusterInfo->len);
            std::string conf(clusterInfo->str);
            initializeCluster(&conf);
        }
        freeReplyObject(clusterInfo);
    }
}

void DataSrcRedisCluster::initializeCluster(std::string * configString){
    std::vector<std::string> confStrs;
    split(*configString, '\n', &confStrs);
    ///< each of vector elements now represents one node in redis cluster
    ///< some of them may be slaves so we`ll dec clients count each time we meet it
    clientsCount = confStrs.size();
    clients = new redisContext*[clientsCount];
    clients[0] = mainClient;
    ///< main clients is always num 0
    int currentNodeNumber = 1;
    for (auto i = confStrs.begin(); i != confStrs.end(); i++){
        std::vector<std::string> confTokens;
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
        if (confTokens[2].find("myself") != std::string::npos){ ///< myself, main client node
            ///< assign slots only
            auto slotIterator = confTokens.begin();
            slotIterator += 7;
            while (slotIterator != confTokens.end()){
                std::string * slotStr = &(*slotIterator);
                if (slotStr[0][0] != '[')
                    if (slotStr->find("-") != std::string::npos){ ///< slot range or one slot?
                        int slotRange[2];
                        std::istringstream ( *slotStr ) >> slotRange[0] >> slotRange[1];
                        slotRange[1] = - slotRange[1];
                        for (int j = slotRange[0]; j <= slotRange[1]; j += 1)
                            slotMap[j] = 0;
                    } else {
                        int slot;
                        std::istringstream ( *slotStr ) >> slot;
                        slotMap[slot] = 0;
                    }
                slotIterator += 1;
            }
        } else {
            if (confTokens[2].find("master") != std::string::npos){
                if (confTokens[6] == "connected"){
                    ///< 1. connecting
                    char ip[30],
                            pos = confTokens[1].find(":");
                    for (size_t j = 0; j < pos; j += 1)
                        ip[j] = confTokens[1][j];
                    ip[pos] = '\0';
                    std::string a = confTokens[1].substr(pos + 1);
                    int port;
                    std::istringstream ( a ) >> port;
                    clients[currentNodeNumber] = redisConnect(ip, port);
                    ///< 2. slots assign
                    auto slotIterator = confTokens.begin();
                    slotIterator += 7;
                    while (slotIterator != confTokens.end()){
                        std::string * slotStr = &(*slotIterator);
                        if (slotStr[0][0] != '[')
                            if (slotStr->find("-") != std::string::npos){ ///< slot range or one slot?
                                int slotRange[2];
                                std::istringstream ( *slotStr ) >> slotRange[0] >> slotRange[1];
                                slotRange[1] = - slotRange[1];
                                for (int j = slotRange[0]; j <= slotRange[1]; j += 1)
                                    slotMap[j] = currentNodeNumber;
                            } else {
                                int slot;
                                std::istringstream ( *slotStr ) >> slot;
                                slotMap[slot] = currentNodeNumber;
                            }
                        slotIterator += 1;
                    }
                    currentNodeNumber += 1;
                } else {
                    error->clear();
                    error->append("One of cluster master nodes is down! Node address: ");
                    error->append(confTokens[2]);
                }
            } else {
                clientsCount -= 1;
                ///< decrement clientsCount because we are faced with slave node
            }
        }
    }
}

void DataSrcRedisCluster::reinitializeCluster(){
    redisReply * clusterInfo = (redisReply *)(redisCommand(mainClient, "cluster nodes"));
    if (clusterInfo->type == REDIS_REPLY_ERROR) {
        error->append("Something wrong with cluster! Errstring: '");
        error->append(clusterInfo->str);
        error->append("'.");
    } else {
        uint16_t newMapping = crc16(clusterInfo->str, clusterInfo->len);
        MUTEX_LOCK(mtx);
        if (newMapping != lastMapping){
            deinitializeCluster();
            std::string s(clusterInfo->str);
            initializeCluster(&s);
            lastMapping = newMapping;
        }
        MUTEX_UNLOCK(mtx);
    }
    freeReplyObject(clusterInfo->str);
}

void DataSrcRedisCluster::deinitializeCluster(){
    for (int i = 0; i < clientsCount; i += 1){
        redisFree(clients[i]);
    }
    delete[] clients;
}

void DataSrcRedisCluster::saveIds(unsigned int docNumber, const unsigned int * hashes, unsigned int count){
    for (int i = 0; i < clientsCount; i += 1){
        redisCommand(clients[i], "multi");
    }
    for (int i = 0; i < count; i += 1){
        char tmp[15];
        sprintf(tmp, "hash:%d", hashes[i]);
        int len = strlen(tmp);
        redisReply * rep= (redisReply*)redisCommand(clients[slotMap[crc16(tmp, len) & 0x3fff]], "sadd %b %b", tmp, len, &docNumber, sizeof(docNumber));
        freeReplyObject(rep);
    }
    for (int i = 0; i < clientsCount; i += 1){
        redisReply * rep= (redisReply*)redisCommand(clients[i], "exec");
        int k =0;
        for (int j = 0; j < rep->elements; j += 1){
            redisReply * aaa = rep->element[j];
            if (aaa->type == REDIS_REPLY_ERROR)
                k+=1;
        }
        freeReplyObject(rep);
    }
}

void DataSrcRedisCluster::saveDocument(DocHeader header, t__text * txt){
    char key[30];
    char tryCount = 0; ///< count of tries already done
    sprintf(key, "document:number:%d", header.number);
    int len = strlen(key);
    redisContext * context = clients[slotMap[crc16(key, len) & 0x3fff]];
    bool flag = false;
    redisReply * rep;
    do {
        redisCommand(context, "multi");
        rep = (redisReply*)redisCommand(context, "hset %b textName %b", key, (size_t) len, txt->name, header.textName_len); ///< textName is a name of field in hash
        if (rep->type == REDIS_REPLY_ERROR){
            tryCount += 1;
            reinitializeCluster();
            if (tryCount < 10)
                continue;
            else
                break;
        }
        freeReplyObject(rep);
        redisCommand(context, "hset %b textData %b", key, (size_t) len, txt->streamData, header.data_len);
        redisCommand(context, "hset %b authorName %b", key, (size_t) len, txt->authorName, header.authorName_len);
        redisCommand(context, "hset %b authorGroup %b", key, (size_t) len, txt->authorGroup, header.authorGroup_len);
        redisCommand(context, "hset %b textType %b", key, (size_t) len, &(txt->type), sizeof(txt->type));
        char * date = asctime(&(header.dateTime));
        redisCommand(context, "hset %b date %b", key, (size_t) len, date, strlen(date));
        rep = (redisReply*)redisCommand(context, "exec");
        tryCount += 1;
        if (rep->type == REDIS_REPLY_ERROR && tryCount < 10){
            reinitializeCluster();
            flag = true;
        } else
            flag = false;
    } while (flag);
    if (tryCount >= 10) {
        error->clear();
        error->append("Error in save document function: ");
        error->append(rep->str);
        error->append("'.");
    }
    freeReplyObject(rep);
}

void DataSrcRedisCluster::getDocument(unsigned int docNumber, t__text *trgt, soap * parent){
    char key[30];
    sprintf(key, "document:number:%d", docNumber);
    int len = strlen(key);
    int contNum = slotMap[crc16(key, len) & 0x3fff];
    redisContext * context = clients[contNum];
    redisReply * rep;
    bool flag = false;
    do {
        rep = (redisReply*)redisCommand(context, "hget %b textName", key, (size_t) len);
        if (rep->type == REDIS_REPLY_ERROR) {
            reinitializeCluster();
            flag = true;
        }
        trgt->name = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
        strcpy(trgt->name, rep->str);
        freeReplyObject(rep);
    } while (flag);

    rep = (redisReply*)redisCommand(context, "hget %b textData", key, (size_t) len);
    trgt->streamData = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
    strcpy(trgt->streamData, rep->str);
    freeReplyObject(rep);

    rep = (redisReply*)redisCommand(context, "hget %b authorName", key, (size_t) len);
    trgt->authorName = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
    strcpy(trgt->authorName, rep->str);
    freeReplyObject(rep);

    rep = (redisReply*)redisCommand(context, "hget %b authorGroup", key, (size_t) len);
    trgt->authorGroup = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
    strcpy(trgt->authorGroup, rep->str);
    freeReplyObject(rep);

    rep = (redisReply*)redisCommand(context, "hget %b textType", key, (size_t) len);
    trgt->type = *((t__type*)(rep->str));
    freeReplyObject(rep);

    rep = (redisReply*)redisCommand(context, "hget %b date", key, (size_t) len);
    trgt->date = reinterpret_cast<char*>(soap_malloc(parent, rep->len + 1));
    strcpy(trgt->date, rep->str);
    freeReplyObject(rep);
}

std::vector<unsigned int> * DataSrcRedisCluster::getIdsByHashes(const unsigned int * hashes, unsigned int count){
    auto result = new std::vector<unsigned int>();
    for (int i = 0; i < count; i += 1){
        char tmp[15];
        sprintf(tmp, "hash:%d", hashes[i]);
        redisReply * rep = (redisReply*)redisCommand(clients[slotMap[crc16(tmp, strlen(tmp)) & 0x3fff]], "smembers %s", tmp);
        for (int j = 0; j < rep->elements; j += 1){
            redisReply * el = rep->element[j];
            result->push_back(*((unsigned int *)(el->str)));
        }
        freeReplyObject(rep);
    }
    return result;
}

DataSrcRedisCluster::~DataSrcRedisCluster(){
    deinitializeCluster();
    delete error;
    MUTEX_CLEANUP(mtx);
}

static const uint16_t crc16tab[256]= {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

uint16_t DePlaguarism::crc16(const char *buf, int len) {
    int counter;
    uint16_t crc = 0;
    for (counter = 0; counter < len; counter++)
            crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *buf++)&0x00FF];
    return crc;
}

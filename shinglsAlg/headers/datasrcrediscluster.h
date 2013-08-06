#ifndef DATASRCREDISCLUSTER_H
#define DATASRCREDISCLUSTER_H
#include "./datasrcberkeleydb.h"

namespace DePlaguarism {

    enum dataSrc__t {DATA_SRC_BDB = 0, DATA_SRC_REDIS_CLUSTER = 1};

    uint16_t crc16(const char *buf, int len);
    std::vector<std::string> *split(const std::string &s, char delim, std::vector<std::string> *elems);

    class DataSrcRedisCluster : public DataSrcAbstract
    {
    private:
        redisContext * mainClient; ///< pointer to main node client; this pointer also in clients[0] !!!
        redisContext ** clients;
        bool main;
        int clientsCount;
        static char * mainClientIp;
        static int mainClientPort;
        static uint8_t slotMap[16384];
        static uint16_t lastMapping; ///< contains crc16 hash of last config
        static MUTEX_TYPE mtx;///< used in reinitializeCluster to prevent remapping
        void reinitializeCluster();
        void deinitializeCluster();
        void redisCommandWithoutReply(redisContext *c, const char *format, ...);
        redisReply * redisCommandWithReply(redisContext *c, const char *format, ...);
        void initializeCluster(std::string * configString, bool remap); ///< configStr is a string in reply from main redis node
    public:
        std::string * error;
        DataSrcRedisCluster(const char * ipAddress, int port, bool main);
        virtual ~DataSrcRedisCluster();
        virtual std::vector<unsigned int> * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text **trgtPtr, soap * parent);
    };

}


#endif // DATASRCREDISCLUSTER_H

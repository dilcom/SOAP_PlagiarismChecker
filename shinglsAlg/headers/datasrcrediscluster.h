#ifndef DATASRCREDISCLUSTER_H
#define DATASRCREDISCLUSTER_H
#include "./datasrcabstract.h"
#include "../include/hiredis.h"
#include "../include/threads.h"
#include <stdint.h>
#include <string>

namespace DePlaguarism {

    uint16_t crc16(const char *buf, int len);

    class DataSrcRedisCluster : public DataSrcAbstract
    {
    private:
        redisContext * mainClient; ///< pointer to main node client; this pointer also in clients[0] !!!
        redisContext ** clients;
        int clientsCount;
        static uint8_t slotMap[16384];
        static uint16_t lastMapping; ///< contains crc16 hash of last config
        static MUTEX_TYPE mtx;///< used in configure to prevent remapping
        void reinitializeCluster();
        void initializeCluster(char *configString); ///< configStr is a string in reply from main redis node
    public:
        std::string * error;
        DataSrcRedisCluster(const char * ipAddress, int port);
        virtual ~DataSrcRedisCluster();
        virtual std::vector<unsigned int> * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void saveIds(unsigned int docNumber, const unsigned int * hashes, unsigned int count);
        virtual void saveDocument(DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text *trgt, soap * parent);
    };

}


#endif // DATASRCREDISCLUSTER_H

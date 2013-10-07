#ifndef DATASRCREDISCLUSTER_H
#define DATASRCREDISCLUSTER_H
#include "./datasrcabstract.h"
#include "../include/hiredis.h"

namespace DePlagiarism {
    std::vector<std::string> *split(const std::string &s, char delim, std::vector<std::string> *elems);

    //! Wrapper class for redis cluster.
    class DataSrcRedisCluster : public DataSrcAbstract
    {
    private:
        redisContext * m_mainClient; ///< Pointer to main node client; this pointer also in m_clients[0] !!!
        redisContext ** m_clients; ///< Array of clients, every one of which has an opened session with cluster node.
        bool m_main; ///< Shows whether it is main client or not.
        int m_clientsCount; ///< Count of master nodes in cluster (size of m_clients).
        void reinitializeCluster(); ///< Renews config and rebuilds the object to satisfy it.
        void deinitializeCluster(); ///< Correctly frees all the memory allocated by object.
        void redisCommandWithoutReply(redisContext *c, const char *format, ...); ///< Throws an exception if command was refused.
        redisReply * redisCommandWithReply(redisContext *c, const char *format, ...); ///< Throws an exception if command was refused.
        //! Initializes cluster.
            /*!
              \param config is a result of "cluster nodes" command, addressed to any node of cluster.
              \param remap tell whether we should build slotMap again or not.
            */
        void initializeCluster(std::string * configString, bool remap); ///< configStr is a string in reply from main redis node
        /* static ! */
        static char * m_mainClientIp; ///< IP of m_mainClient.
        static int m_mainClientPort; ///< Port number of m_mainClient.
        static uint8_t slotMap[16384]; ///< Map which shows which node will serve request.
        static uint16_t m_lastMapping; ///< Contains crc16 hash of last config.
        static MUTEX_TYPE m_mtx; ///< Used in reinitializeCluster to prevent remapping.
    public:
        DataSrcRedisCluster(const char * ipAddress, int port, bool main);
        virtual ~DataSrcRedisCluster();
        virtual getIdsByHashesResult__t * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text **trgtPtr, soap * parent);
    };

}


#endif // DATASRCREDISCLUSTER_H

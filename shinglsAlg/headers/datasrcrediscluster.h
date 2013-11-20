#ifndef DATASRCREDISCLUSTER_H
#define DATASRCREDISCLUSTER_H
#include "./datasrcabstract.h"
#include "../include/hiredis.h"

namespace DePlagiarism {
    std::vector<std::string> *split(const std::string &s, char delim, std::vector<std::string> *elems);

    void * processRedisQuerries(void * client);
    //! Wrapper class for redis cluster.
    class DataSrcRedisCluster : public DataSrcAbstract
    {
        //! Contains tasks in form that can be used by another thread
        class Task {
            //! Just a structure to contain everything we need to do one request.
            struct BasicQuerry {
                char * m_command;
                int m_len;
                bool m_needResult;
                redisContext * m_cont;
                BasicQuerry(redisContext * cont, bool needResult, const char * format, va_list args)
                    :m_needResult(needResult), m_cont(cont){
                    m_len = redisvFormatCommand(&m_command, format, args);
                }
                ~BasicQuerry() { free(m_command); }
            };
        private:
            std::vector<BasicQuerry*> m_querries;
            std::vector<redisReply*> m_result;
            bool m_ready;
            bool m_error;
            MUTEX_TYPE m_mtx;
            COND_TYPE m_cond;
            friend void * DePlagiarism::processRedisQuerries(void * client);
        public:
            Task();
            ~Task();
            /*!
             *\brief Adds simple querry into list holded by Task
              \param cont is a redis context that will handle the request
              \param needResult shows whether result of the command will be returned or not
              \param format is format string for vargs following it
            */
            void addQuerry(redisContext * cont, bool needResult, const char * format, ...);
            /*!
             *\brief Adds result into Task`s result list
              \param rep is a redis reply returned by context
            */
            void addResult(redisReply * rep);
            /*!
             *\brief Used to getResult when Task is complete
              \return Const pointer to result vector
            */
            const std::vector<redisReply*> * getResult();
            /*!
              Signals to thread, that waits for result in waitForResult that task is done.
              \param error shows whether an error occurred or not
            */
            void signalReady(bool error);
            /*!
              Makes thread to wait until task is complete.
              \return true if everything OK otherwise false
            */
            bool waitForResult();
            void clearResult(); ///< Deletes results
            void clearTasks(); ///< Deletes querries
            void clear(); ///< clearResult + clearTasks
        };

    private:
        int threadID; ///< Thread id of worker that use this object.
        THREAD_TYPE m_clientThread; ///< Thread of worker that use this object.
        friend void * DePlagiarism::processRedisQuerries(void * client);
        redisContext * m_mainClient; ///< Pointer to main node client; this pointer also in m_clients[0] !!!
        redisContext ** m_clients; ///< Array of clients, every one of which has an opened session with cluster node.
        int m_clientsCount; ///< Count of master nodes in cluster (size of m_clients).
        char * m_mainClientIp; ///< IP of m_mainClient.
        int m_mainClientPort; ///< Port number of m_mainClient.
        uint8_t slotMap[16384]; ///< Map which shows which node will serve request.
        uint16_t m_lastMapping; ///< Contains crc16 hash of last config.
        ConcurrentQueue<Task*> m_dbQueue; ///< Queue of tasks. One thread must perfrom them.
        void reinitializeCluster(); ///< Renews config and rebuilds the object to satisfy it.
        void deinitializeCluster(); ///< Correctly frees all the memory allocated by object.
        void redisCommandWithoutReply(redisContext *c, const char *format, ...); ///< Throws an exception if command was refused.
        redisReply * redisCommandWithReply(redisContext *c, const char *format, ...);
        /*!
         *\brief Used by asynchro worker to perform requests to redis
          \param c is a redis context that will handle the request
          \param cmd is a command formatted as redis command (result of redisFormat)
          \param len is length of command
        */
        redisReply * getRedisReply(redisContext *c, char *cmd, size_t len);
        //! Initializes cluster.
            /*!
              \param config is a result of "cluster nodes" command, addressed to any node of cluster.
              \param remap tell whether we should build slotMap again or not.
            */
        void initializeCluster(std::string * configString); ///< configStr is a string in reply from main redis node
        //! Stops worker thread (adds NULL into queue, when worker will pop it he will stop).
        void stop();
    public:
        DataSrcRedisCluster(const char * ipAddress, int port, const char *threadName);
        virtual ~DataSrcRedisCluster();
        virtual getIdsByHashesResult__t * getIdsByHashes(const unsigned int * hashes, unsigned int count);
        virtual void save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt);
        virtual void getDocument(unsigned int docNumber, t__text **trgtPtr, soap * parent);
    };

}


#endif // DATASRCREDISCLUSTER_H

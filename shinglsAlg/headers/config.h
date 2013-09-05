#ifndef CONFIG_H
#define CONFIG_H

#ifdef BERKELEYDB
    #include "datasrcberkeleydb.h"
#else
    #include "datasrcrediscluster.h"
#endif
#include <libconfig.h++>

namespace DePlaguarism {
    class Config {
    // singleton
    private:
        Config(){}
        Config(const Config & src){}
        Config& operator=(const Config&){}
    public:
        static Config & getInstance(){
            static Config INSTANCE;
            return INSTANCE;
        }
        
        void loadConfig(const char * configFile);
        void saveConfig(const char * configFile);
        // ShingleApp.h
        float THRESHOLD_TO_SAVE; ///< maximum value from which new texts won`t be added to the database
        unsigned int DOCUMENTS_IN_RESPONSE;///< maximum count of documents which will be responced to client
        unsigned int CONNECTIONS_BEFORE_RESET; ///< frequency between soap->destroy calls
    
        // interfaces
        std::string REDIS_MAIN_CLIENT_ADDRESS;
        unsigned int REDIS_MAIN_CLIENT_PORT;
        std::string GSOAP_IF; ///< interface for gsoap
        unsigned int SERVICE_PORT; ///< port used for soap server
    
        // storage bdb
        std::string HASH_DB_NAME;
        std::string DOCS_DB_NAME;
        std::string ENV_NAME;
    };
    
    namespace DefaultValues {
        const std::string CONFIG_FILE = "shingle_app.conf";
        const float THRESHOLD_TO_SAVE = 0.6; ///< maximum value from which new texts won`t be added to the database
        const unsigned int DOCUMENTS_IN_RESPONSE = 10;///< maximum count of documents which will be responced to client
        const unsigned int CONNECTIONS_BEFORE_RESET = 50; ///< frequency between soap->destroy calls
        const char REDIS_MAIN_CLIENT_ADDRESS [] = "127.0.0.1";
        const unsigned int REDIS_MAIN_CLIENT_PORT = 6379;
        const char GSOAP_IF []= "0.0.0.0"; ///< interface for gsoap
        const unsigned int SERVICE_PORT = 9999; ///< port used for soap server
        const std::string HASH_DB_NAME = "hash.db";
        const std::string DOCS_DB_NAME = "docs.db";
        const std::string ENV_NAME = "./db";
        
        // constants
        const unsigned int MAX_SHINGLE_PER_TEXT = 120; ///< max Shingles in text
        const unsigned int WORDS_EACH_SHINGLE = 4; ///< words in each Shingle
        const unsigned int MIN_WORD_LENGTH = 3; ///< min word length to be processed
        const bool LOG_EVERY_FCALL = false;    
        
        // multithreading constants
        const unsigned int BACKLOG = 100;	///< Max request backlog
        const unsigned int MAX_THR = 10; ///< Size of thread pool
        const unsigned int MAX_QUEUE = 1000; ///< Max. size of request queue    
    }
}


#endif

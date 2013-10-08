#ifndef CONFIG_H
#define CONFIG_H

#include "./precompiled.h"
#include <libconfig.h++>

namespace DePlagiarism {
//! Singleton class.
    /*!
      We have access to its instance from anywhere in application.
      All constructors and operator= are private to forbid creation of extra instances.
      This class also stores information needed by loggers
    */
    class Config {
    private:
        Config(){}
        Config(const Config & src){}
        Config & operator=(const Config&){}
        /*! 
         * \brief Searches for value in config
         * \param target is pointer to variable where we want to store config value
         * \param name is name of value in config file
         * \param defaultValue is value that will be assigned to target if not found in file
         */
        template <class T>
        void lookup(T *target, const char *name, const T &defaultValue); 
        /*! 
         * \brief special realization of lookup for string values
         */
        void lookup(std::string *target, const char *name, const std::string &defaultValue);
        libconfig::Config conf;
    public:
        //! Returns the only object of that class.
        static Config & getInstance(){
            static Config INSTANCE;
            return INSTANCE;
        }

        //! Loads config from file.
            /*!
              \param configFile is relative path to the text file.
            */
        void loadConfig(const char * configFile);
        
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
        // logging constants
        const std::string LOG_FILE_NAME = "shingle_app.log";
        // default values for config
        const std::string CONFIG_FILE = "shingle_app.conf";
        const float THRESHOLD_TO_SAVE = 0.6; ///< maximum value from which new texts won`t be added to the database
        const unsigned int DOCUMENTS_IN_RESPONSE = 10;///< maximum count of documents which will be responced to client
        const unsigned int CONNECTIONS_BEFORE_RESET = 50; ///< frequency between soap->destroy calls
        const std::string REDIS_MAIN_CLIENT_ADDRESS = "127.0.0.1";
        const unsigned int REDIS_MAIN_CLIENT_PORT = 6379;
        const std::string GSOAP_IF = "0.0.0.0"; ///< interface for gsoap
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

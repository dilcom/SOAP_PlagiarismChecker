#ifndef CONSTANTS_H
#define CONSTANTS_H

///< crossplatform define

#ifdef BERKELEYDB
    #include "datasrcberkeleydb.h"
#else
    #include "datasrcrediscluster.h"
#endif


namespace DePlaguarism{

    //Shingle.h
    const unsigned int MAX_SHINGLE_PER_TEXT = 120; ///< max Shingles in text
    const unsigned int WORDS_EACH_SHINGLE = 4; ///< words in each Shingle
    const unsigned int MIN_WORD_LENGTH = 3; ///< min word length to be processed

    //ShingleApp.h
    const float THRESHOLD_TO_SAVE = 0.6; ///< maximum value from which new texts won`t be added to the database
    const unsigned int DOCUMENTS_IN_RESPONSE = 10;///< maximum count of documents which will be responced to client
    const unsigned int CONNECTIONS_BEFORE_RESET = 50; ///< frequency between soap->destroy calls

    const char REDIS_MAIN_CLIENT_ADDRESS [] = "127.0.0.1";
    const unsigned int REDIS_MAIN_CLIENT_PORT = 6379;
    const char GSOAP_IF []= "127.0.0.1"; ///< interface for gsoap
    const unsigned int SERVICE_PORT = 9999; ///< port used for soap server

    //multithreading constants
    const unsigned int BACKLOG = 100;	///< Max request backlog
    const unsigned int MAX_THR  = 10; ///< Size of thread pool
    const unsigned int MAX_QUEUE = 1000; ///< Max. size of request queue

    //storage
    const char HASH_DB_NAME[] = "hash.db";
    const char DOCS_DB_NAME[] = "docs.db";
    const char ENV_NAME[] = "./db";

    //logger mode const
    const bool LOG_EVERY_FCALL = false;

}


//mode constants
//#define WITH_LOGGING ///< enables logging for function invokes

#endif

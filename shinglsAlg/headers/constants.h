#pragma once


///< crossplatform define
#ifdef _WIN32
	#include <Windows.h>
    #define MAKE_DIR(x) CreateDirectory(L##x, NULL)
	#define SLEEP(x) Sleep(x)
    #define PAUSE_STRING "pause"
#else
    #include <sys/stat.h>
    #define MAKE_DIR(x) mkdir(x, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
    #define SLEEP(x) usleep(x*1000)
    #define PAUSE_STRING "read -p \"Press any key to continue ...\" -n 1"
#endif

namespace DePlaguarism{
	//Shingle.h
    const unsigned int MAX_SHINGLE_PER_TEXT = 120; ///< max Shingles in text
    const unsigned int WORDS_EACH_SHINGLE = 4; ///< words in each Shingle
    const unsigned int MIN_WORD_LENGTH = 3; ///< min word length to be processed

	//ShingleApp.h
	const float THRESHOLD_TO_SAVE = 0.6; ///< maximum value from which new texts won`t be added to the database
	const unsigned int DOCUMENTS_IN_RESPONSE = 10;///< maximum count of documents which will be responced to client
    const unsigned int SERVICE_PORT = 9999; ///< port used for soap server
    const unsigned int CONNECTIONS_BEFORE_RESET = 64; ///< frequency between soap->destroy calls
    //multithreading constants
    const unsigned int BACKLOG = 100;	///< Max request backlog
    const unsigned int MAX_THR  = 10; ///< Size of thread pool
    const unsigned int MAX_QUEUE = 1000; ///< Max. size of request queue

	const char HASH_DB_NAME[] = "hash.db";
	const char DOCS_DB_NAME[] = "docs.db";
	const char ENV_NAME[] = "./db";
	
	//logger mode const
    const bool LOG_EVERY_FCALL = true;
}


//mode constants
//#define MODE_DO_NOT_SAVE_RESULTS // database file will not be saved
#define WITH_LOGGING ///< enables logging for function invokes


#include "../headers/config.h"

using namespace DePlaguarism;

/*void Config::saveConfig(const char *configName) {
    libconfig::Config conf;
    try {
        conf.lookup("responce.treshold_to_save")= (long)THRESHOLD_TO_SAVE;
        conf.lookup("responce.documents_in_responce") = (long)DOCUMENTS_IN_RESPONSE;
        conf.lookup("applicaation.connections_before_reset") = (long)CONNECTIONS_BEFORE_RESET;
        conf.lookup("redis.port") = (long)REDIS_MAIN_CLIENT_PORT;
        conf.lookup("application.port") = (long)SERVICE_PORT;
        conf.lookup("application.multithreading.backlog") = (long)BACKLOG;
        conf.lookup("application.multithreading.thread_count") = (long)MAX_THR;
        conf.lookup("application.multithreading.queue_size") = (long)MAX_QUEUE;
        conf.lookup("redis.address") = REDIS_MAIN_CLIENT_ADDRESS;
        conf.lookup("application.address") = GSOAP_IF;
        conf.lookup("berkeleydb.hash_db_name") = HASH_DB_NAME;
        conf.lookup("berkeleydb.docs_db_name") = DOCS_DB_NAME;
        conf.lookup("berkeleydb.env_name") = ENV_NAME;
        conf.writeFile(configName);
    } catch (...) {
        //???
    }
}*/

void Config::loadConfig(const char * configName) {
    libconfig::Config conf;
    try {
        conf.readFile(configName);
        THRESHOLD_TO_SAVE = conf.lookup("treshold_to_save");
        DOCUMENTS_IN_RESPONSE = conf.lookup("documents_in_responce");
        CONNECTIONS_BEFORE_RESET = conf.lookup("connections_before_reset");
        REDIS_MAIN_CLIENT_PORT = conf.lookup("redis_port");
        SERVICE_PORT = conf.lookup("port");
        REDIS_MAIN_CLIENT_ADDRESS = (std::string)conf.lookup("redis_address").c_str();
        GSOAP_IF = conf.lookup("address").c_str();
        HASH_DB_NAME = conf.lookup("hash_db_name").c_str();
        DOCS_DB_NAME = conf.lookup("docs_db_name").c_str();
        ENV_NAME = conf.lookup("env_name").c_str();
    } catch (...) {
        //set default variables
        THRESHOLD_TO_SAVE = DePlaguarism::DefaultValues::THRESHOLD_TO_SAVE;
        DOCUMENTS_IN_RESPONSE = DePlaguarism::DefaultValues::DOCUMENTS_IN_RESPONSE;
        CONNECTIONS_BEFORE_RESET = DePlaguarism::DefaultValues::CONNECTIONS_BEFORE_RESET;    
        REDIS_MAIN_CLIENT_ADDRESS = DePlaguarism::DefaultValues::REDIS_MAIN_CLIENT_ADDRESS;
        REDIS_MAIN_CLIENT_PORT = DePlaguarism::DefaultValues::REDIS_MAIN_CLIENT_PORT;
        GSOAP_IF = DePlaguarism::DefaultValues::GSOAP_IF;
        SERVICE_PORT = DePlaguarism::DefaultValues::SERVICE_PORT; 
        HASH_DB_NAME = DePlaguarism::DefaultValues::HASH_DB_NAME;
        DOCS_DB_NAME = DePlaguarism::DefaultValues::DOCS_DB_NAME;
        ENV_NAME = DePlaguarism::DefaultValues::ENV_NAME;
    }
}

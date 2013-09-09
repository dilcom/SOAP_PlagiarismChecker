#include "../headers/config.h"

using namespace DePlaguarism;
using namespace std;
void Config::loadConfig(const char * configName) {
    libconfig::Config conf;
    try {
        conf.readFile(configName);
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
        return;
    }
    try {
        THRESHOLD_TO_SAVE = conf.lookup("treshold_to_save");
    } catch (...) {
        //set default variable
        THRESHOLD_TO_SAVE = DePlaguarism::DefaultValues::THRESHOLD_TO_SAVE;
    }
    try {
        ENV_NAME = conf.lookup("env_name").c_str();
    } catch (...) {
        //set default variable
        ENV_NAME = DePlaguarism::DefaultValues::ENV_NAME;
    }
    try {
        DOCS_DB_NAME = conf.lookup("docs_db_name").c_str();
    } catch (...) {
        //set default variable
        DOCS_DB_NAME = DePlaguarism::DefaultValues::DOCS_DB_NAME;
    }
    try {
        HASH_DB_NAME = conf.lookup("hash_db_name").c_str();
    } catch (...) {
        //set default variable
        HASH_DB_NAME = DePlaguarism::DefaultValues::HASH_DB_NAME;
    }
    try {
        GSOAP_IF = conf.lookup("address").c_str();
    } catch (...) {
        //set default variable
        GSOAP_IF = DePlaguarism::DefaultValues::GSOAP_IF;
    }
    try {
        REDIS_MAIN_CLIENT_ADDRESS = (string)conf.lookup("redis_address").c_str();
    } catch (...) {
        //set default variable
        REDIS_MAIN_CLIENT_ADDRESS = DePlaguarism::DefaultValues::REDIS_MAIN_CLIENT_ADDRESS;
    }
    try {
        REDIS_MAIN_CLIENT_PORT = conf.lookup("redis_port");
    } catch (...) {
        //set default variable
        REDIS_MAIN_CLIENT_PORT = DePlaguarism::DefaultValues::REDIS_MAIN_CLIENT_PORT;
    }
    try {
        SERVICE_PORT = conf.lookup("port");
    } catch (...) {
        //set default variable
        SERVICE_PORT = DePlaguarism::DefaultValues::SERVICE_PORT; 
    }
    try {
        CONNECTIONS_BEFORE_RESET = conf.lookup("connections_before_reset");
    } catch (...) {
        //set default variable
        CONNECTIONS_BEFORE_RESET = DePlaguarism::DefaultValues::CONNECTIONS_BEFORE_RESET;    
    }
    try {
        DOCUMENTS_IN_RESPONSE = conf.lookup("documents_in_responce");
    } catch (...) {
        //set default variable
        DOCUMENTS_IN_RESPONSE = DePlaguarism::DefaultValues::DOCUMENTS_IN_RESPONSE;
    }
}

#include "../headers/config.h"

using namespace DePlagiarism;
using namespace std;

template <class T>
void Config::lookup(T *target, const char *name, const T &defaultValue){
    try {
        *target = m_conf.lookup(name);
    } catch (...) {
        *target = defaultValue;
    }
}
void Config::lookup(string *target, const char *name, const string &defaultValue){
    try {
        *target = (string)(m_conf.lookup(name).c_str());
    } catch (...) {
        *target = defaultValue;
    }
}


void Config::loadConfig(const char * configName) {
    try {
        m_conf.readFile(configName);
    } catch (...) {}
    lookup(&THRESHOLD_TO_SAVE, "treshold_to_save", DefaultValues::THRESHOLD_TO_SAVE);
    lookup(&ENV_NAME, "env_name", DefaultValues::ENV_NAME);
    lookup(&DOCS_DB_NAME, "docs_db_name", DefaultValues::DOCS_DB_NAME);
    lookup(&HASH_DB_NAME, "hash_db_name", DefaultValues::HASH_DB_NAME);
    lookup(&GSOAP_IF, "address", DefaultValues::GSOAP_IF);
    lookup(&REDIS_MAIN_CLIENT_ADDRESS, "redis_address", DefaultValues::REDIS_MAIN_CLIENT_ADDRESS);
    lookup(&REDIS_MAIN_CLIENT_PORT, "redis_port", DefaultValues::REDIS_MAIN_CLIENT_PORT);
    lookup(&SERVICE_PORT, "port", DefaultValues::SERVICE_PORT);
    lookup(&CONNECTIONS_BEFORE_RESET, "connections_before_reset", DefaultValues::CONNECTIONS_BEFORE_RESET);
    lookup(&DOCUMENTS_IN_RESPONSE, "documents_in_response", DefaultValues::DOCUMENTS_IN_RESPONSE);
}

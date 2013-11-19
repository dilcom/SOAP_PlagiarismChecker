#include "../headers/ShingleApp.h"
using namespace DePlagiarism;
using namespace std;

typedef vector<unsigned int>::const_iterator vecConstIt;
typedef map<unsigned int, unsigned int>::const_iterator mapConstIt;

DataSrcAbstract * ShingleApp::m_dataSource;
ConcurrentQueue<SOAP_SOCKET> ShingleApp::m_clientRequests(DefaultValues::MAX_QUEUE);

bool ShingleApp::validateText(t__text * a){
    if (a->streamData == NULL)
        return false;
    if (a->authorGroup == NULL) {
        a->authorGroup = (char*)(soap_malloc(a->creator, 10));
        strcpy(a->authorGroup, "NoGroup\0");
    }
    if (a->authorName == NULL) {
        a->authorName = (char*)(soap_malloc(a->creator, 10));
        strcpy(a->authorName, "NoAuName\0");
    }
    if (a->name == NULL) {
        a->name = (char*)(soap_malloc(a->creator, 11));
        strcpy(a->name, "NoTxtName\0");
    }
    if (a->date == NULL) {
        a->date = (char*)(soap_malloc(a->creator, 8));
        strcpy(a->date, "NoDate\0");
    }
    return true;
}

string ShingleApp::ipToStr(){
    string res("");
    char str[16];
    unsigned char * charIp = (unsigned char*)(&(this->ip));
    sprintf(str, "%3d.%3d.%3d.%3d", charIp[3], charIp[2], charIp[1], charIp[0]);
    res.append(str);
    return res;
}


void ShingleApp::setMain(){
    m_mainEx = true;
    sprintf(m_threadName, "Acceptor");
    m_logger = Log::getLogger();
}

void ShingleApp::setChild(int i){
    m_mainEx = false;
    sprintf(m_threadName, "Worker N%d", i);
    m_logger = Log::getLogger();
}

ShingleApp::ShingleApp(void)
{
    setMain();
    m_flagContinue = true; // once turned to false it will stop an application after next socket accept
    loadDB();
}

ShingleApp::~ShingleApp(void)
{	
    if (m_mainEx)
        closeDB();
}

int ShingleApp::CompareText(t__text * txt, result * res){
    if (validateText(txt))
        switch (txt->type) {
        case TEXT:
            return shingleAlgorithm(txt, res);
        case PROGRAMM_LANG_CPP: ;
        case PROGRAMM_LANG_PASCAL: ;
        case CLEAR_TEXT: ;
        }
    return SOAP_ERR;
}

void ShingleApp::findSimilar(t__text * txt){
    clock_t time = - clock();
    map<unsigned int, unsigned int> fResult;
    Shingle * tested = new Shingle(*txt);
    m_appResult.clear();
    try{
        unsigned int cnt = tested->getCount();
        // first -- extract from data source documents with same hashes
        const unsigned int * data = tested->getData();
        getIdsByHashesResult__t * docIds = m_dataSource->getIdsByHashes(data, tested->getCount());
        // now we have vector of document ids that have same hashes as our document
        // second -- count repeatings; because of huge count of ids we use a map
        for (vecConstIt i = docIds->begin(); i != docIds->end(); i++){
            unsigned int el = *(i);
            mapConstIt it = fResult.find(el);
            if (it != fResult.end()) // is there already same docid in the map?
                fResult[el] = fResult[el] + 1; // inc
            else
                fResult[el] = 1;
        }
        // now we have a map with pairs (document id; count of equal hashes)
        // third -- create a vector sorted by count of equal hashes
        for (map<unsigned int, unsigned int>::iterator it = fResult.begin(); it != fResult.end(); it++){
            Pair * tmpPtr;
            m_appResult.push_back( *(tmpPtr = new Pair(it->first, (float)(it->second)/cnt)) );
            delete tmpPtr;
        }
        sort(m_appResult.begin(), m_appResult.end(), objectcomp);
        // now we have a sorted vector of pairs, so the job is done
        // fourth -- finally we should think about storing new document in DB
        if (m_appResult.empty() || m_appResult[0].similarity <= Config::getInstance().THRESHOLD_TO_SAVE) {
            tested->save(m_dataSource);
        }
        delete docIds;
    }
    catch(...){
        m_logger->error("{%s}: ERROR in ShingleApp::findSimilar", m_threadName);
    }
    m_logger->debug("{%s}: Execution of ShingleApp::findSimilar took %d msecs", m_threadName, time + clock());
    delete tested;
}

int ShingleApp::shingleAlgorithm(t__text * txt, result *res){
    clock_t time = - clock();
    m_logger->debug("{%s}: Request from %s recieved", m_threadName, ipToStr().c_str());
    findSimilar(txt);
    int cnt = min(m_appResult.size(), (size_t)Config::getInstance().DOCUMENTS_IN_RESPONSE);
    res->arrayOfTexts.reserve(cnt);
    for (int j = 0; j < cnt; j += 1){
        t__text * textElement = new t__text();
        m_dataSource->getDocument(m_appResult[j].docId, &textElement, this);
        if (textElement != NULL) {
            textElement->similarity = m_appResult[j].similarity;
            res->arrayOfTexts.push_back(*textElement);
            delete textElement;
        } else {
            cnt -= 1;
        }
    }
    res->errCode = res->arrayOfTexts.empty() ? STATE_NO_SIMILAR : STATE_OK;
    m_logger->debug("{%s}: Execution of ShingleApp::shingleAlgorithm took %d msecs", m_threadName, time + clock());
    m_logger->debug("{%s}: Text size: %d bytes", m_threadName, (int)(sizeof(char)*strlen(txt->streamData)));
    return SOAP_OK;
}

bool operator==(const Pair & left, const Pair & right)
{ 
    return left.docId == right.docId;
}

bool ClassComp::operator() (const Pair & left, const Pair & right) const
{
    return left.similarity > right.similarity;
}

void ShingleApp::stop(){
    m_flagContinue = false;
}

int ShingleApp::run(int port){
    ShingleApp *soap_thr[DefaultValues::MAX_THR]; // each thread needs a runtime context
    THREAD_TYPE tid[DefaultValues::MAX_THR];
    SOAP_SOCKET m, s;
    unsigned int i;
    m = this->bind(Config::getInstance().GSOAP_IF.c_str(), port, DefaultValues::BACKLOG);
    if (!soap_valid_socket(m)){
        m_logger->error("{%s}: Connection error! Port may be busy!", m_threadName);
        return 1;
    }
    m_logger->debug("{%s}: Socket connection successful %d", m_threadName, m);
    // 1. we create runtime contexts for threads and run them
    for (i = 0; i < DefaultValues::MAX_THR; i++) {
        unsigned threadID;
        soap_thr[i] = new ShingleApp(*this);
        soap_thr[i]->setChild(i);
        m_logger->notice("{%s}: Starting thread %d", m_threadName, i);
        THREAD_CREATE(&tid[i], process_queue, (void*)soap_thr[i], &threadID);
    }
    // 2. We`re accepting every connection on our port and putting it into queue
    while(true) {
        s = this->accept();
        if (!m_flagContinue)
            break;
        if (!soap_valid_socket(s))
        {
            if (this->errnum)
            {
                m_logger->error("{%s}: Invalid socket recieved.", m_threadName);
                continue; // retry
            }
            else
            {
                m_logger->error("{%s}: Server timed out", m_threadName);
                break;
            }
        }
        m_logger->debug("{%s}: One more socket %d connection from IP %s", m_threadName, s, ipToStr().c_str());
        // SOAP_EOM means the following: "too many connections in queue, please wait for a slot, sir"
        while (!m_clientRequests.push(s))
            SLEEP(10);
    }

    // 3. Now we want to stop all the threads, so lets fill the queue with invalid sockets and threads will stop itself
    for (i = 0; i < DefaultValues::MAX_THR; i++) {
        while (!m_clientRequests.push(SOAP_INVALID_SOCKET))
            SLEEP(10);
    }
    // 4. Waiting for all the threads to end
    for (i = 0; i < DefaultValues::MAX_THR; i++) {
        m_logger->notice("{%s}: Waiting for thread %d to terminate... ", m_threadName, i);
        THREAD_JOIN(tid[i]);
        m_logger->notice("{%s}: ...terminated", m_threadName);
        delete soap_thr[i];
    }
    return 0;
}

log4cpp::Category & ShingleApp::getLogger() {
    return *m_logger;
}


void *DePlagiarism::process_queue(void *soap)
{
    ShingleApp *tsoap = static_cast<ShingleApp*>(soap);
    unsigned int i = 0;
    while (true) {
        tsoap->socket = ShingleApp::m_clientRequests.pop();
        if (!soap_valid_socket(tsoap->socket))
            break;
        unsigned int time = - clock();
        tsoap->serve();
        time += clock();
        tsoap->getLogger().debug("{%s}: Served in %d msecs", tsoap->m_threadName, (time/1000));
        i += 1;
        if (i == Config::getInstance().CONNECTIONS_BEFORE_RESET){
            i = 0;
            tsoap->getLogger().debug("{%s}: Soap::destroy start", tsoap->m_threadName);
            time = - clock();
            tsoap->destroy(); // deallocates all the memory allocated in soap_malloc(..)
            tsoap->getLogger().debug("{%s}: Soap::destroy ends in %d msecs", tsoap->m_threadName, time + clock());
        }
    }
    return SOAP_OK;
}

void ShingleApp::loadDB() {
    bool flag = false;
    do {
        flag = false;
        try{
#ifdef BERKELEYDB
            m_dataSource = new DataSrcBerkeleyDB(Config::getInstance().ENV_NAME.c_str(), Config::getInstance().HASH_DB_NAME.c_str(), Config::getInstance().DOCS_DB_NAME.c_str(), m_mainEx, m_threadName);
#else
            m_dataSource = new DataSrcRedisCluster(Config::getInstance().REDIS_MAIN_CLIENT_ADDRESS.c_str(), Config::getInstance().REDIS_MAIN_CLIENT_PORT, m_threadName);
#endif
        }
        catch (...){
            flag = true;
            m_logger->error("{%s}: Database opening error! Trying to reconnect...", m_threadName);
            SLEEP(1000);
        }
    } while (flag && m_flagContinue);
}

void ShingleApp::closeDB(){
    if (m_dataSource)
        delete m_dataSource;
}

#include "../headers/ShingleApp.h"
using namespace DePlagiarism;
using namespace std;

typedef vector<unsigned int>::const_iterator vecConstIt;
typedef map<unsigned int, unsigned int>::const_iterator mapConstIt;

// Multithread handling globals
SOAP_SOCKET queue[DefaultValues::MAX_QUEUE]; // The global request queue of sockets
unsigned int head = 0, tail = 0; // Queue head and tail
COND_TYPE queue_cv; // used only in socket queuing
MUTEX_TYPE queue_mx; // used only in socket queuing
// Multithread handling end

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

string ShingleApp::nowToStr(){
    string res;
    char buffer[80];
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo = localtime(&rawtime);
    strftime (buffer, sizeof(buffer), "%X %x", timeinfo);
    res = buffer;
    return res;
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
}

void ShingleApp::setChild(){
    m_mainEx = false;
}

ShingleApp::ShingleApp(void)
{
    setMain();
    m_Log = new ShingleAppLogger();
    //Log->addLogFile("log.txt");
    loadDB();
}

ShingleApp::~ShingleApp(void)
{	
    closeDB();
    if (m_mainEx){
        delete m_Log;
    }
}

ShingleAppLogger & ShingleApp::log(){
    return *m_Log;
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
        *m_Log << "!!!ERROR in ShingleApp::findSimilar\n";
    }
    if (DefaultValues::LOG_EVERY_FCALL){
        *m_Log << "ShingleApp::findSimilar execution took " << (int)(time + clock()) << "msec\n";
    }

    delete tested;
}

int ShingleApp::shingleAlgorithm(t__text * txt, result *res){
    clock_t time = - clock();
    *m_Log << "Request from " << ipToStr() << " recieved\n";
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
    if (DefaultValues::LOG_EVERY_FCALL){
        *m_Log << "Request processed in " << (int)(time + clock()) << "msec\n";
        *m_Log << "Text size: " << (int)(sizeof(char)*strlen(txt->streamData)) << " bytes\n\n";
    }
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
    m_flagContinue = true; // once turned to false it will stop an application after next socket accept
    ShingleApp *soap_thr[DefaultValues::MAX_THR]; // each thread needs a runtime context
    THREAD_TYPE tid[DefaultValues::MAX_THR];
    SOAP_SOCKET m, s;
    unsigned int i;
    m = this->bind(Config::getInstance().GSOAP_IF.c_str(), port, DefaultValues::BACKLOG);
    if (!soap_valid_socket(m)){
        *m_Log << "Connection error! Port may be busy!\n";
        return 1;
    }
    *m_Log << "Socket connection successful" << m << "\n";
    COND_SETUP(queue_cv);
    MUTEX_SETUP(queue_mx);
    // 1. we create runtime contexts for threads and run them
    for (i = 0; i < DefaultValues::MAX_THR; i++) {
        unsigned threadID;
        soap_thr[i] = new ShingleApp(*this);
        soap_thr[i]->setChild();
        soap_thr[i]->loadDB();
        *m_Log << "Starting thread " << i << "\n";
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
                soap_print_fault(stderr);
                continue; // retry
            }
            else
            {
                *m_Log << "Server timed out\n";
                break;
            }
        }
        *m_Log << "Thread " << i << " accepts socket " << s << " connection from IP " << ipToStr() << "\n";
        // SOAP_EOM means the following: "too many connections in queue, please wait for a slot, sir"
        while (enqueue(s) == SOAP_EOM)
            SLEEP(1);
    }

    // 3. Now we want to stop all the threads, so lets fill the queue with invalid sockets and threads will stop itself
    for (i = 0; i < DefaultValues::MAX_THR; i++) {
        while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
            SLEEP(1);
    }
    // 4. Waiting for all the threads to end
    for (i = 0; i < DefaultValues::MAX_THR; i++) {
        *m_Log << "Waiting for thread " << i <<" to terminate... ";
        THREAD_JOIN(tid[i]);
        *m_Log << "terminated\n";
        delete soap_thr[i];
    }
    COND_CLEANUP(queue_cv);
    MUTEX_CLEANUP(queue_mx);
    return 0;
}

#ifdef WIN32
unsigned _stdcall DePlagiarism::process_queue(void *soap)
#else
void *DePlagiarism::process_queue(void *soap)
#endif
{
    ShingleApp *tsoap = static_cast<ShingleApp*>(soap);
    unsigned int i = 0;
    while (true) {
        tsoap->socket = dequeue();
        if (!soap_valid_socket(tsoap->socket))
            break;
        unsigned int time = - clock();
        tsoap->serve();
        time += clock();
        tsoap->log() << "Served in " << (time/1000) << "msecs \n";
        i += 1;
        if (i == Config::getInstance().CONNECTIONS_BEFORE_RESET){
            i = 0;
            tsoap->destroy(); // deallocates all the memory allocated in soap_malloc(..)
        }
    }
    return SOAP_OK;
}

int DePlagiarism::enqueue(SOAP_SOCKET sock) {
    int status = SOAP_OK;
    unsigned int next;
    MUTEX_LOCK(queue_mx);
    next = tail + 1;
    if (next >= DefaultValues::MAX_QUEUE)
        next = 0;
    if (next == head)
        status = SOAP_EOM;
    else
    {
        queue[tail] = sock;
        tail = next;
    }
    COND_SIGNAL(queue_cv);
    MUTEX_UNLOCK(queue_mx);
    return status;
}

SOAP_SOCKET DePlagiarism::dequeue() {
    SOAP_SOCKET sock;
    MUTEX_LOCK(queue_mx);
    while (head == tail)
        COND_WAIT(queue_cv, queue_mx);
    sock = queue[head++];
    if (head >= DefaultValues::MAX_QUEUE)
        head = 0;
    MUTEX_UNLOCK(queue_mx);
    return sock;
}


void ShingleApp::loadDB() {
    bool flag = false;
    do {
        flag = false;
        try{
#ifdef BERKELEYDB
            m_dataSource = new DataSrcBerkeleyDB(Config::getInstance().ENV_NAME.c_str(), Config::getInstance().HASH_DB_NAME.c_str(), Config::getInstance().DOCS_DB_NAME.c_str(), m_mainEx);
#else
            m_dataSource = new DataSrcRedisCluster(Config::getInstance().REDIS_MAIN_CLIENT_ADDRESS.c_str(), Config::getInstance().REDIS_MAIN_CLIENT_PORT, m_mainEx);
#endif
        }
        catch (...){
            flag = true;
            *m_Log << "Database opening error! Trying to reconnect...\n";
            m_Log->flush();
            SLEEP(1000);
        }
    } while (flag);
}

void ShingleApp::closeDB(){
    if (m_dataSource)
        delete m_dataSource;
}

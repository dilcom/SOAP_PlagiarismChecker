#include "../headers/stdafx.h"
#include "../headers/ShingleApp.h"
using namespace DePlaguarism;

bool DePlaguarism::txtValid(t__text * a){
	return (a->streamData && a->authorGroup && a->authorName && a->name);
}


int ShingleApp::documentCount;///< count of document already stored in base
MUTEX_TYPE ShingleApp::mtx;///< crossplatform mutex

string ShingleApp::nowToStr(){
	string res;
	time_t a;
	time(&a);
	res = asctime(localtime(&a));
	return res;
}

string ShingleApp::ipToStr(){
	string res;
	char str[16];
	unsigned char * charIp = (unsigned char*)(&(this->ip));
	sprintf(str, "%3d.%3d.%3d.%3d", charIp[3], charIp[2], charIp[1], charIp[0]);
	res.append(str);
	return res;
}


void ShingleApp::setMain(){
    mainEx = true;
}

void ShingleApp::setChild(){
    mainEx = false;
}

ShingleApp::ShingleApp(void)
{
    setMain();
    Log = new ShingleAppLogger();
	ifstream f;
	f.open("docNumber.t");
	if (f)
		f.read((char*)(&documentCount), sizeof(documentCount));
	else
		documentCount = 0;
	f.close();
    Log->addTrgt(&cout);
	MUTEX_SETUP(mtx);
    Log->addLogFile("log.txt");
    loadDB();
}

ShingleApp::~ShingleApp(void)
{	
    if (mainEx){
        ofstream f;
        f.open("docNumber.t");
        f.write((char*)(&documentCount), sizeof(documentCount));
        f.close();
        delete Log;
		MUTEX_CLEANUP(mtx);
    }
    closeDB();
}

ShingleAppLogger & ShingleApp::log(){
    return *Log;
};

using namespace std;
using namespace DePlaguarism;

int ShingleApp::CompareText(t__text * txt, t__result * res){
    if (txtValid(txt))
        switch (txt->type) {
            case TEXT:
                return shingleAlgorithm(txt, res);
		}
	return SOAP_ERR;
}


void ShingleApp::findSimilar(t__text * txt){
	clock_t time = - clock();
	map<unsigned int, unsigned int> fResult;
    Shingle * tested = new Shingle(txt, 0);
	appResult.clear();
    try{
		unsigned int cnt = tested->getCount();
        unsigned int currentDocId;
        ///< first -- extract from data source documents with same hashes
        auto data = tested->getData();
        std::vector<unsigned int> * docIds = dataSource->getIdsByHashes(data, tested->getCount());
        ///< now we have vector of document ids that have same hashes as our document
        ///< second -- count repeatings; because of huge count of ids we use a map
        for (auto i = docIds->begin(); i != docIds->end(); i++){
            unsigned int el = *(i);
            auto it = fResult.find(el);
            if (it != fResult.end()) ///< is there already same docid in the map?
                fResult[el] = fResult[el] + 1; ///< inc
            else
                fResult[el] = 1;
        }
        ///< now we have a map with pairs (document id; count of equal hashes)
        ///< third -- create a vector sorted by count of equal hashes
		for (map<unsigned int, unsigned int>::iterator it = fResult.begin(); it != fResult.end(); it++){
            Pair * tmpPtr;
            appResult.push_back( *(tmpPtr = new Pair(it->first, (float)(it->second)/cnt)) );
            delete tmpPtr;
		}
		sort(appResult.begin(), appResult.end(), objectcomp);
        ///< now we have a sorted vector of pairs, so the job is done
        ///< fourth -- finally we should think about storing new document in DB
        if (appResult.empty() || appResult[0].similarity <= THRESHOLD_TO_SAVE) {
        	MUTEX_LOCK(mtx);
			documentCount += 1;
			int tmpk = documentCount;
        	MUTEX_UNLOCK(mtx);
            tested->save(dataSource, tmpk);
        }
        delete docIds;
	}
	catch(...){
        *Log << "!!!ERROR in ShingleApp::findSimilar\n";
	}
	if (LOG_EVERY_FCALL){
        *Log << "ShingleApp::findSimilar execution took " << (int)(time + clock()) << "msec\n";
	}

	delete tested;
}

int ShingleApp::shingleAlgorithm(t__text * txt, t__result *res){
	clock_t time = - clock();
    *Log << "Request from " << ipToStr() << " recieved\n";
	findSimilar(txt);
    int cnt = min(appResult.size(), (size_t)DOCUMENTS_IN_RESPONSE);
    res->errCode = cnt ? STATE_OK : STATE_NO_SIMILAR;
	res->arrayOfTexts.reserve(cnt);
	for (int j = 0; j < cnt; j += 1){
        t__text * textElement = new t__text();
        dataSource->getDocument(appResult[j].docId, textElement, this);
		textElement->similarity = appResult[j].similarity;
        res->arrayOfTexts.push_back(*textElement);
        textElement->streamData = NULL;
        delete textElement;
	}
	if (LOG_EVERY_FCALL){
        *Log << "Request processed in " << (int)(time + clock()) << "msec\n";
        *Log << "Text size: " << (int)(sizeof(char)*strlen(txt->streamData)) << " bytes\n\n";
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
    flagContinue = false;
}


SOAP_SOCKET queue[MAX_QUEUE]; ///< The global request queue of sockets
int head = 0, tail = 0; // Queue head and tail
COND_TYPE queue_cv;
MUTEX_TYPE queue_mx;


int ShingleApp::run(int port){
    flagContinue = true;
    ShingleApp *soap_thr[MAX_THR]; // each thread needs a runtime context
    THREAD_TYPE tid[MAX_THR];
    SOAP_SOCKET m, s;
    int i;
    m = this->bind(NULL, port, BACKLOG);
    if (!soap_valid_socket(m)){
		*Log << "Connection error! Port may be busy!\n";
        return 1;
	}
    fprintf(stderr, "Socket connection successful %d\n", m);
    COND_SETUP(queue_cv);
    MUTEX_SETUP(queue_mx);
    for (i = 0; i < MAX_THR; i++)
    {
		unsigned threadID;
        soap_thr[i] = new ShingleApp(*this);
        soap_thr[i]->setChild();
        soap_thr[i]->loadDB();
        fprintf(stderr, "Starting thread %d\n", i);
        THREAD_CREATE(&tid[i], process_queue, (void*)soap_thr[i], &threadID);
    }
    for (;;)
    {
        s = this->accept();
        if (!flagContinue)
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
                fprintf(stderr, "Server timed out\n");
                break;
            }
        }
        fprintf(stderr, "Thread %d accepts socket %d connection from IP %d.%d.%d.%d\n", i, s, (this->ip >> 24)&0xFF, (this->ip >> 16)&0xFF, (this->ip >> 8)&0xFF, this->ip&0xFF);
        while (enqueue(s) == SOAP_EOM)
            SLEEP(1);
    }
    for (i = 0; i < MAX_THR; i++)
    {
        while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
            SLEEP(1);
        SLEEP(10);
    }
    for (i = 0; i < MAX_THR; i++)
    {
        fprintf(stderr, "Waiting for thread %d to terminate... ", i);
        THREAD_JOIN(tid[i]);
        fprintf(stderr, "terminated\n");
        soap_thr[i]->destroy();
		delete soap_thr[i];
    }
    COND_CLEANUP(queue_cv);
    MUTEX_CLEANUP(queue_mx);
    soap_done(this);
    return 0;
 }

#ifdef WIN32
	unsigned _stdcall DePlaguarism::process_queue(void *soap)
#else
	void *DePlaguarism::process_queue(void *soap)
#endif
{
   ShingleApp *tsoap = static_cast<ShingleApp*>(soap);
   for (unsigned int i = 0; 1; i+=1)
   {
      tsoap->socket = dequeue();
      if (!soap_valid_socket(tsoap->socket))
         break;
      tsoap->serve();
      fprintf(stderr, "served\n");
      if (i % 64 == 0) tsoap->destroy();
   }
   return SOAP_OK;
}

int DePlaguarism::enqueue(SOAP_SOCKET sock)
{
   int status = SOAP_OK;
   int next;
   MUTEX_LOCK(queue_mx);
   next = tail + 1;
   if (next >= MAX_QUEUE)
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

SOAP_SOCKET DePlaguarism::dequeue()
{
   SOAP_SOCKET sock;
   MUTEX_LOCK(queue_mx);
   while (head == tail)
       COND_WAIT(queue_cv, queue_mx);
   sock = queue[head++];
   if (head >= MAX_QUEUE)
      head = 0;
   MUTEX_UNLOCK(queue_mx);
   return sock;
}


void ShingleApp::loadDB(){
    try{
        dataSource = new DataSrcRedisCluster("127.0.0.1", 6379);
        //dataSource = new DataSrcBerkeleyDB(ENV_NAME, HASH_DB_NAME, DOCS_DB_NAME, this->mainEx);
    }
    catch (...){
        ///< TODO exception catching
        *Log << "Database opening error!\n";
    }
}

void ShingleApp::closeDB(){
    delete dataSource;
}

void ShingleApp::resetDB(){
    *Log << "ResetDB invoked!\n";
    closeDB();
    loadDB();
}

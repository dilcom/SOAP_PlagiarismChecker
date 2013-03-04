#include "../headers/stdafx.h"
#include "../headers/ShingleApp.h"
using namespace DePlaguarism;

bool DePlaguarism::txtValid(t__text a){
	return (a.streamData && a.authorGroup && a.authorName && a.name);
}

DbEnv * ShingleApp::env; ///< database in BDB
Db * ShingleApp::hashes; ///< bdb table, contains pairs hash => doc_id
Db * ShingleApp::docs;	///< bdb table, contains pairs doc_id => documentInfo

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

void ShingleApp::initTextById(unsigned int id, t__text * trgt){
	clock_t time = - clock();
	try{
        Dbc *cursorp;
        Dbt key(&id, sizeof(id) );
        Dbt dataItem(0, 0);
        //key.set_flags(DB_DBT_REALLOC);
        //dataItem.set_flags(DB_DBT_REALLOC);
        docs->cursor(NULL, &cursorp, 0);        
        cursorp->get(&key, &dataItem, DB_SET);
		char * pointer = (char *)(dataItem.get_data());
		DocHeader header = *(DocHeader *) pointer;
		trgt->type = header.type;
		pointer += sizeof(DocHeader);

        trgt->authorGroup = reinterpret_cast<char*>(soap_malloc(this, header.authorGroup_len + 1));
		memcpy(trgt->authorGroup, pointer, header.authorGroup_len);
		trgt->authorGroup[header.authorGroup_len] = '\0';
		pointer += header.authorGroup_len;

		trgt->authorName = reinterpret_cast<char*>(soap_malloc(this, header.authorName_len + 1));
		memcpy(trgt->authorName, pointer, header.authorName_len);
		trgt->authorName[header.authorName_len] = '\0';
		pointer += header.authorName_len;

		trgt->streamData = reinterpret_cast<char*>(soap_malloc(this, header.data_len + 1));
		memcpy(trgt->streamData, pointer, header.data_len);
		trgt->streamData[header.data_len] = '\0';
		pointer += header.data_len;

		trgt->name = reinterpret_cast<char*>(soap_malloc(this, header.textName_len + 1));
		memcpy(trgt->name, pointer, header.textName_len);;
		trgt->name[header.textName_len] = '\0';
		pointer += header.textName_len;

		char * date = asctime(&(header.dateTime));
		trgt->date = reinterpret_cast<char*>(soap_malloc(this, strlen(date) + 1));
        strcpy(trgt->date, date);
        cursorp->close();
        //free(dataItem.get_data());
        //free(key.get_data());
	}
	catch(...){
        *Log << "!!!ERROR in ShingleApp::initTextById \n";
		return;
	}
	
	if (LOG_EVERY_FCALL){
        *Log << "ShingleApp::initTextById execution took " << (int)(time + clock()) << "msec\n";
	}
}


void ShingleApp::setMain(){
    mainEx = true;
}

void ShingleApp::setChild(){
	documentCount = 0;
    mainEx = false;
}

ShingleApp::ShingleApp(void)
{
    Log = new ShingleAppLogger();
	ifstream f;
	f.open("docNumber.t");
	if (f)
		f.read((char*)(&documentCount), sizeof(documentCount));
	else
		documentCount = 0;
	f.close();
    Log->addTrgt(&cout);
    loadDB();
    Log->addLogFile("log.txt");
}

ShingleApp::~ShingleApp(void)
{	
    if (mainEx){
        ofstream f;
        f.open("docNumber.t");
        f.write((char*)(&documentCount), sizeof(documentCount));
        f.close();
        delete Log;
        closeDB();
    }
}

ShingleAppLogger & ShingleApp::log(){
    return *Log;
};

using namespace std;
using namespace DePlaguarism;

int ShingleApp::CompareText(t__text txt, t__result * res){
    if (txtValid(txt))
        switch (txt.type) {
            case TEXT:
                return shingleAlgorithm(txt, res);
		}
	return SOAP_ERR;
}


void ShingleApp::findSimilar(t__text & txt){
	clock_t time = - clock();
	map<unsigned int, unsigned int> fResult;
	Shingle * tested = new Shingle(txt, documentCount);
	Dbc *cursorp;
	appResult.clear();
    try{
		unsigned int cnt = tested->getCount();
		unsigned int currentDocId;
        //env->mutex_lock(*mutexDB);
        //*Log << "!!!";
        hashes->cursor(NULL, &cursorp,  0);
        //*Log << "!!!";
		for (unsigned int i = 0; i < cnt; i++){
			Dbt key((void*)(tested->getData() + i), sizeof(unsigned int));
            Dbt data(&currentDocId, sizeof(currentDocId));
            //key.set_flags(DB_DBT_REALLOC);
            //data.set_flags(DB_DBT_REALLOC);
			int ret = cursorp->get(&key, &data, DB_SET);
			while (ret != DB_NOTFOUND) {
				unsigned int dt = *((unsigned int*)(data.get_data()));
				map<unsigned int, unsigned int>::iterator it = fResult.find(dt);
				if (it != fResult.end())
					fResult[dt] = fResult[dt] + 1;
				else fResult[dt] = 1;
				ret = cursorp->get(&key, &data, DB_NEXT_DUP);
            }
            //free(data.get_data());
            //free(key.get_data());
        }
        cursorp->close();
        //env->mutex_unlock(*mutexDB);
		unsigned int shCount = tested->getCount();
		for (map<unsigned int, unsigned int>::iterator it = fResult.begin(); it != fResult.end(); it++){
            Pair * tmpPtr;
            appResult.push_back( *(tmpPtr = new Pair(it->first, (float)(it->second)/shCount)) );
            delete tmpPtr;
		}
		sort(appResult.begin(), appResult.end(), objectcomp);
        if (appResult.empty() || appResult[0].similarity <= THRESHOLD_TO_SAVE) {
            tested->save(docs, hashes);
			documentCount += 1;
		}	
	}
	catch(...){
        *Log << "!!!ERROR in ShingleApp::findSimilar\n";
	}
	if (LOG_EVERY_FCALL){
        *Log << "ShingleApp::findSimilar execution took " << (int)(time + clock()) << "msec\n";
	}

	delete tested;
}

int ShingleApp::shingleAlgorithm(t__text txt, t__result *res){
	clock_t time = - clock();
    *Log << "Request from " << ipToStr() << " recieved\n";
	findSimilar(txt);
    int cnt = min(appResult.size(), (size_t)DOCUMENTS_IN_RESPONSE);
    res->errCode = cnt ? STATE_OK : STATE_NO_SIMILAR;
	for (int j = 0; j < cnt; j += 1){
		t__text * textElement = new t__text();
		initTextById(appResult[j].docId, textElement);
		textElement->similarity = appResult[j].similarity;
        res->arrayOfTexts.push_back(*textElement);
        delete textElement;
	}
	if (LOG_EVERY_FCALL){
        *Log << "Request processed in " << (int)(time + clock()) << "msec\n";
        *Log << "Text size: " << (int)(sizeof(char)*strlen(txt.streamData)) << " bytes\n\n";
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
MUTEX_TYPE queue_cs;
COND_TYPE queue_cv;



int ShingleApp::run(int port){
    flagContinue = true;
    ShingleApp *soap_thr[MAX_THR]; // each thread needs a runtime context
    THREAD_TYPE tid[MAX_THR];
    SOAP_SOCKET m, s;
    int i;
    m = this->bind(NULL, port, BACKLOG);
    if (!soap_valid_socket(m))
        exit(1);
    fprintf(stderr, "Socket connection successful %d\n", m);
    MUTEX_SETUP(queue_cs);
    COND_SETUP(queue_cv);
    for (i = 0; i < MAX_THR; i++)
    {
        soap_thr[i] = new ShingleApp(*this);
        soap_thr[i]->setChild();
        fprintf(stderr, "Starting thread %d\n", i);
        THREAD_CREATE(&tid[i], process_queue, (void*)soap_thr[i]);
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
    }
    for (i = 0; i < MAX_THR; i++)
    {
        fprintf(stderr, "Waiting for thread %d to terminate... ", i);
        THREAD_JOIN(tid[i]);
        fprintf(stderr, "terminated\n");
		documentCount += soap_thr[i]->documentCount;
		delete soap_thr[i];
    }
    MUTEX_CLEANUP(queue_cs);
    COND_CLEANUP(queue_cv);
    soap_done(this);
    return 0;
 }

void *DePlaguarism::process_queue(void *soap)
{
   ShingleApp *tsoap = static_cast<ShingleApp*>(soap);
   for (;;)
   {
      tsoap->socket = dequeue();
      if (!soap_valid_socket(tsoap->socket))
         break;
      tsoap->serve();
      fprintf(stderr, "served\n");
   }
   tsoap->destroy();
   return SOAP_OK;
}

int DePlaguarism::enqueue(SOAP_SOCKET sock)
{
   int status = SOAP_OK;
   int next;
   MUTEX_LOCK(queue_cs);
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
   MUTEX_UNLOCK(queue_cs);
   return status;
}

SOAP_SOCKET DePlaguarism::dequeue()
{
   SOAP_SOCKET sock;
   MUTEX_LOCK(queue_cs);
   while (head == tail)
       COND_WAIT(queue_cv, queue_cs);
   sock = queue[head++];
   if (head >= MAX_QUEUE)
      head = 0;
   MUTEX_UNLOCK(queue_cs);
   return sock;
}

void ShingleApp::stat(){
    env->stat_print(0);
}

void ShingleApp::compactDB(){
    log() << "DB compact operation begin\n";
    DbTxn * envTXN;
    env->cdsgroup_begin(&envTXN);
    docs->compact(envTXN, NULL, NULL, NULL, DB_FREE_SPACE, NULL);
    hashes->compact(envTXN, NULL, NULL, NULL, DB_FREE_SPACE, NULL);
    log() << "DB compact operation end\n";
}

void ShingleApp::loadDB(){
    try{
        env = new DbEnv(0);
        //env->set_flags(DB_CDB_ALLDB, 1);
        env->open(ENV_NAME, DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_THREAD, 0);
        hashes = new Db(env, 0);
        docs = new Db(env, 0);
        hashes->set_flags(DB_DUP);
        hashes->open(0, HASH_DB_NAME, NULL, DB_HASH, DB_CREATE | DB_THREAD, 0644);
        docs->open(0, DOCS_DB_NAME, NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0644);
    }
    catch (...){
        ///< TODO exception catching
        *Log << "Database opening error!\n";
    }
}

void ShingleApp::closeDB(){
    hashes->close(0);
    docs->close(0);
    env->close(0);
    delete hashes;
    delete docs;
    delete env;
}

void ShingleApp::resetDB(){
    *Log << "ResetDB invoked!\n";
    closeDB();
    loadDB();
}

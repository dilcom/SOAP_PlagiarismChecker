#include "../headers/ShingleApp.h"

using namespace DePlaguarism;

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
	_snprintf(str, 16, "%d.%d.%d.%d", charIp[3], charIp[2], charIp[1], charIp[0]);
	res = str;
	return res;
}

void ShingleApp::initTextById(unsigned int id, t__text * trgt){
	clock_t time = - clock();
	try{
		Dbc *cursorp;
		docs->cursor(NULL, &cursorp, 0);
		Dbt key(&id, sizeof(id) );
		Dbt dataItem(0, 0);
		int ret = cursorp->get(&key, &dataItem, DB_SET);
		char * pointer = (char *)(dataItem.get_data());
		trgt->type = *(t__type *) pointer;
		pointer += sizeof(t__type);
		int len = strlen(pointer);
		trgt->authorGroup = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(trgt->authorGroup, pointer);
		pointer += len + 1;
		len = strlen(pointer);
		trgt->authorName = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(trgt->authorName, pointer);
		pointer += len + 1;
		len = strlen(pointer);
		trgt->streamData = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(trgt->streamData, pointer);
		pointer += len + 1;
		len = strlen(pointer);
		trgt->name = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(trgt->name, pointer);
		pointer += len + 1;
		char * date = asctime((tm*)pointer);
		len = strlen(date);
		trgt->date = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(trgt->date, date);
	}
	catch(...){
		Log << "!!!ERROR in ShingleApp::initTextById \n";
		return;
	}
	
	if (LOG_EVERY_FCALL){
		Log << "ShingleApp::initTextById execution took " << time + clock() << "msec\n";
	}
}


ShingleApp::ShingleApp(void)
{
	ifstream f;
	f.open("docNumber.t");
	if (f)
		f.read((char*)(&documentCount), sizeof(documentCount));
	else
		documentCount = 0;
	f.close();
	Log.addTrgt(&cout);
	try{
		env = new DbEnv(0);	
		env->open(ENV_NAME, DB_CREATE | DB_INIT_MPOOL, 0);
		hashes = new Db(env, 0);
		docs = new Db(env, 0);
		hashes->set_flags(DB_DUP);
		hashes->open(0, HASH_DB_NAME, NULL, DB_HASH, DB_CREATE, 0644);
		docs->open(0, DOCS_DB_NAME, NULL, DB_BTREE, DB_CREATE, 0644);
	}
	catch (...){
		///< TODO exception catching
		Log << "Database opening error!" << '\n';
		//this->~ShingleApp();
	}
	Log.addLogFile("log.txt");
}

ShingleApp::~ShingleApp(void)
{
	ofstream f;
	f.open("docNumber.t");
	f.write((char*)(&documentCount), sizeof(documentCount));
	f.close();
	hashes->close(0);
	docs->close(0);
	env->close(0);
	delete hashes;
	delete docs;
	delete env;
}

ShingleAppLogger & ShingleApp::log(){
	return Log;	
};

using namespace std;
using namespace DePlaguarism;

int ShingleApp::CompareText(t__text txt, t__result * res){
	switch (txt.type) {
		case TEXT: 
			return shingleAlgorithm(txt, res);
			break;
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
		hashes->cursor(NULL, &cursorp, 0);
		unsigned int cnt = tested->getCount();
		unsigned int currentDocId;
		for (unsigned int i = 0; i < cnt; i++){
			Dbt key((void*)(tested->getData() + i), sizeof(unsigned int));
			Dbt data(&currentDocId, sizeof(currentDocId));
			int ret = cursorp->get(&key, &data, DB_SET);
			while (ret != DB_NOTFOUND) {
				unsigned int dt = *((unsigned int*)(data.get_data()));
				map<unsigned int, unsigned int>::iterator it = fResult.find(dt);
				if (it != fResult.end())
					fResult[dt] = fResult[dt] + 1;
				else fResult[dt] = 1;
				ret = cursorp->get(&key, &data, DB_NEXT_DUP);
			}
		}
		unsigned int shCount = tested->getCount();
		for (map<unsigned int, unsigned int>::iterator it = fResult.begin(); it != fResult.end(); it++){
			appResult.push_back( *(new Pair(it->first, (float)(it->second)/shCount)) );
		}
		sort(appResult.begin(), appResult.end(), objectcomp);
		if (!appResult.size()) appResult.push_back(Pair(0, 0));
		if (appResult[0].similarity <= addingMax) {
			tested->save(docs, hashes);
			documentCount += 1;
		}	
	}
	catch(...){
		Log << "!!!ERROR in ShingleApp::findSimilar\n";
	}
	if (LOG_EVERY_FCALL){
		Log << "ShingleApp::findSimilar execution took " << time + clock() << "msec\n";
	}
	delete tested;
}

int ShingleApp::shingleAlgorithm(t__text txt, t__result *res){
	Log << "Request from " << ipToStr() << " recieved\n";
	clock_t time = - clock();
	findSimilar(txt);
	int i = 0;
	int arrSize = appResult.size();
	while (appResult[i].similarity >= similarity && i < 10 && i < arrSize - 1)
		i += 1;
	if (appResult[i].similarity < similarity)
		i -= 1;
	res->cnt = i + 1;
	res->errCode = res->cnt ? STATE_OK : STATE_NO_SIMILAR;
	res->__size = res->cnt;
	res->__ptr = new t__text[res->cnt];
	for (int j = 0; j <= i; j += 1){
		res->__ptr[i].similarity = appResult[i].similarity;
		initTextById(appResult[i].docId, res->__ptr + i);
	}
	if (LOG_EVERY_FCALL){
		Log << "Request processed\n\n";
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
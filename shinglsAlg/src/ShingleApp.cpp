#include "../headers/stdafx.h"
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
	//delete hashes;
	//delete docs;
	//delete env;
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
	}	
	return SOAP_ERR;
}


void ShingleApp::findSimilar(t__text & txt){
	clock_t time = - clock();
	map<unsigned int, unsigned int> fResult;
	Shingle * tested = 
		new Shingle(txt, documentCount);
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
		if (appResult[0].similarity <= THRESHOLD_TO_SAVE) {
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
	int cnt = min(appResult.size(), DOCUMENTS_IN_RESPONCE);
	res->errCode = cnt ? STATE_OK : STATE_NO_SIMILAR;
	for (int j = 0; j < cnt; j += 1){
		t__text * textElement = new t__text();
		initTextById(appResult[j].docId, textElement);
		textElement->similarity = appResult[j].similarity;
		res->arrayOfTexts.push_back(*textElement);
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
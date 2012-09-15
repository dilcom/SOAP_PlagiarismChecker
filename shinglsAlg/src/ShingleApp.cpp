#include "../headers/ShingleApp.h"

void ShingleApp::initializeResponce(int sampNum, t__text * result){
	vector<shingleAppStaff::ShingleAppText>::iterator it = textsArray.begin();
	while (((*it).number != sampNum) && (it != textsArray.end()))
		it++;
	if (it->number == sampNum){
		int len=it->name.length();
		result->name = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(result->name, it->name.c_str());
		len=it->authorName.length();
		result->authorName = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(result->authorName, it->authorName.c_str());
		len=it->authorGroup.length();
		result->authorGroup = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(result->authorGroup, it->authorGroup.c_str());
		result->streamData = NULL;
		result->type = it->type;
		char * buff = asctime(&(it->dateTime));
		len = strlen(buff);
		result->date = reinterpret_cast<char*>(soap_malloc(this, len+1));
		strcpy(result->date, buff);

		ifstream in;
		char nmbr[20];
		_itoa(sampNum, nmbr, 10);
		strcat(nmbr, "article.text");
		in.open(nmbr);
		int fileSize;
		in.read((char*)(&fileSize), sizeof(fileSize));
		result->streamData = reinterpret_cast<char*>(soap_malloc(this, fileSize + 1));
		in.read(result->streamData, fileSize);
		(result->streamData)[fileSize] = '\0';
		in.close();
	}
}


ShingleApp::ShingleApp(void)
{
	Log = new ShingleAppLogger();
	Log->addTrgt(&cout);
	//Log->addLogFile("log.txt");
}

ShingleApp::~ShingleApp(void)
{
	delete Log;
}

ShingleAppLogger * ShingleApp::log(){
	return Log;	
};

using namespace std;
using namespace shingleAppStaff;

int ShingleApp::CompareText(t__text txt, t__result * res){
	switch (txt.type) {
		case /*t__type::*/TEXT: 
			return compareShingles(txt, res);
	}	
	return SOAP_ERR;
}

void ShingleApp::saveSampls(){
	ofstream out;
	out.open("articles.txt", ios::out);
	int cnt = textsArray.size();
	out.write((char *) (&(cnt)), sizeof(cnt));
    for(vector<shingleAppStaff::ShingleAppText>::iterator it = textsArray.begin(); it != textsArray.end(); it++ ) {
		out << it->name << '|' << it->authorName << '|' << it->authorGroup << '|';
		out.write((char *)(&(it->number)), sizeof(int));
		out.write((char *)(&(it->type)), sizeof(t__type));
		out.write((char *)(&(it->dateTime)), sizeof(tm));
    }
	log()->toLog("Texts saved!").endLine();
	out.close();
}

void ShingleApp::loadSampls(){
	textsArray.clear();
	ifstream in;
	in.open("articles.txt", ios::in);
	int cnt;
	if (!in){
		log()->toLog("Base is empty!").endLine();
		return;
	}
	in.read((char *) (&cnt), sizeof(cnt) );
	int i = 0;
	try{
		for(i = 0; i < cnt; i++ ) {
			ShingleAppText *it = new ShingleAppText();
			char ch,
				buff[60];
			int pos = 0;
			while ( (ch = in.get()) != '|') {
				buff[pos] = ch;
				pos++;
			}
			buff[pos] = '\0';
			it->name = new char[pos + 1];
			it->name = buff;
			pos = 0;
			while ( (ch = in.get()) != '|') {
				buff[pos] = ch;
				pos++;
			}
			buff[pos] = '\0';
			it->authorName = new char[pos + 1];
			it->authorName = buff;
			pos = 0;
			while ( (ch = in.get()) != '|') {
				buff[pos] = ch;
				pos++;
			}
			buff[pos] = '\0';
			it->authorGroup = new char[pos + 1];
			it->authorGroup = buff;
			in.read((char *)(&(it->number)), sizeof(int));
			in.read((char *)(&(it->type)), sizeof(t__type));
			in.read((char *)(&(it->dateTime)), sizeof(tm));
			textsArray.push_back(*it);
			delete it;
		}
	}
	catch (exception e){
		log()->toLog("Exception caught while reading ").toLog(i).toLog("-th sampl. Description:").endLine().toLog(e.what()).endLine();
	}
	
	in.close();
	int size = textsArray.size();
	log()->toLog(size).toLog(size == 1 ? " text" :" texts").toLog(" loaded!").endLine();
}

float * ShingleApp::findSimilar(char *txt){
	int cnt = textsArray.size();
	float * result = new float[cnt];
	Shingle * tested = new Shingle(txt);
	bool flag = true;
	for (int i = 0; i < cnt; i++){
		result[i] = tested->compareWith(*(new Shingle(i)));
		if (result[i] > addingMax) flag = false;
	}
	if (flag)
		tested->saveToFile(textsArray.size());
	delete tested;
	return result;
}

int ShingleApp::compareShingles(t__text txt, t__result *res){
	ofstream out;
	char nmbr[20];
	_itoa(textsArray.size(), nmbr, 10);
	strcat(nmbr, "article.text");
	out.open(nmbr);
	int fileSize = strlen(txt.streamData);
	out.write((char *) (&fileSize), sizeof(fileSize));
	out.write(txt.streamData, strlen(txt.streamData));
	out.close();

	clock_t timeEllapsed = - clock();
	log()->toLog("Request recieved from ").toLogIp(this->ip).endLine();

	setlocale(0, "ru_RU.UTF-8");
	this->mode = this->mode | SOAP_C_UTFSTRING;
	res->errCode = STATE_OK;
	res->cnt = 0;
	int cnt = textsArray.size();

	if (cnt == 0){
		Shingle a(txt.streamData);
		a.saveToFile(textsArray.size());
		ShingleAppText * m = new ShingleAppText(txt);
		m->number = textsArray.size();
		textsArray.push_back(*m);
		res->errCode = STATE_BASE_EMPTY;
		delete m;
	}
	else{ //base isnt empty
		float * resArr = findSimilar(txt.streamData);
		float firstMax = resArr[0];
		int firstMaxNum = 0;
		for (int i = 1; i < cnt; i++)
			if (firstMax < resArr[i]){
				firstMax = resArr[i];
				firstMaxNum = i;
			}
			if (firstMax > similarity){
				res->cnt++;
				res->res1 = firstMax;
				initializeResponce(firstMaxNum, &(res->text1));
				resArr[firstMaxNum] = 0;
				float secondMax = resArr[0];
				int secondMaxNum = 0;
				for (int i = 1; i < cnt; i++)
					if (secondMax < resArr[i]){
						secondMax = resArr[i];
						secondMaxNum = i;
					}
					if (secondMax > similarity && cnt > 1){
						res->cnt++;
						res->res2 = secondMax;
						initializeResponce(secondMaxNum, &(res->text2));
						resArr[secondMaxNum] = 0;
						float thirdMax = resArr[0];
						int thirdMaxNum = 0;
						for (int i = 1; i < cnt; i++)
							if (thirdMax < resArr[i]){
								thirdMax = resArr[i];
								thirdMaxNum = i;
							}
							if (thirdMax > similarity && cnt > 2){
								res->cnt++;
								res->res3 = secondMax;
								initializeResponce(thirdMaxNum, &(res->text3));
							}
					}
			}
			else res->errCode = STATE_NO_SIMILAR;
			if (firstMax < addingMax) {
				ShingleAppText * m = new ShingleAppText(txt);
				m->number = textsArray.size();
				textsArray.push_back(*m);
				delete m;
			}
			delete resArr;
	}

	#ifndef MODE_DO_NOT_SAVE_RESULTS
		saveSampls();
	#endif

	timeEllapsed += clock();

	log()->toLog("Request processing took ").toLog(timeEllapsed).toLog(" msec").endLine();

	return SOAP_OK;
}
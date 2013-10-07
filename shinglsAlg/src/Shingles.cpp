#include "../headers/Shingles.h"
#ifdef _WIN32
#pragma comment (lib, "../lib/libdb53.lib")
#endif
using namespace std;
using namespace DePlagiarism;

wstring * utf8to16(char * src){
    vector <unsigned short> utf16result;
    utf8::utf8to16(src, src + strlen(src), back_inserter(utf16result));
    wstring * ws = new wstring(utf16result.begin(), utf16result.end());
    return ws;
}

char * utf16to8(wstring src){
    char * utf8result = new char[src.length() * 2 + 2];
    vector <unsigned short> utf16src;
    for (unsigned int i = 0; i < src.length(); i++)
        utf16src.push_back(src[i]);
    *(utf8::utf16to8(utf16src.begin(), utf16src.end(), utf8result)) = '\0';
    return utf8result;
}

void wstrToLower(wstring * ws){
    transform(ws->begin(), ws->end(), ws->begin(), towlower);
}

Shingle::Shingle(void)
{
}

Shingle::Shingle(const t__text & inText)
{
    //canonization
    m_textData = new t__text(inText);

    wstring *ptrtxt = utf8to16 (m_textData->streamData);
    wstring txt = *ptrtxt;
    wstrToLower(&txt);
    wchar_t *buff = new wchar_t[txt.length() + 1];
    size_t posTxt = 0,
            posBuff = 0,
            wordCount = 0,
            curWordLength = 0,
            lastSpacePos = -1;
    size_t txtLength = txt.length();
    while ( posTxt < txtLength && !iswalpha( txt[posTxt] ))
        posTxt++;
    while (posTxt < txtLength){
        if (txt[posTxt] == L' ' || txt[posTxt] == L'\n'){
            if (curWordLength > DefaultValues::MIN_WORD_LENGTH ){
                buff[posBuff] = L' ';
                lastSpacePos = posBuff;
                curWordLength = 0;
                wordCount++;
                posBuff++;
            }
            else{
                posBuff = lastSpacePos + 1;
                curWordLength = 0;
            }
        }
        if (iswalpha(txt[posTxt])){
            buff[posBuff] = txt[posTxt];
            curWordLength++;
            posBuff++;
        }
        posTxt++;
    }
    buff[posBuff] = L'\0';
    //end of canonization
    if (posBuff != 0)
        wordCount += 1;
    int * words = new int[wordCount + 10];
    words[0] = 0;
    unsigned int posWords = 1,
            shingleCount = max(1, (int)(wordCount - DefaultValues::WORDS_EACH_SHINGLE + 1));
    m_count = min(shingleCount, DefaultValues::MAX_SHINGLE_PER_TEXT);
    for (size_t i = 0; i < posBuff; i++){
        if (buff[i] == ' ')
            words[posWords++] = i;
    }
    words[posWords] = posBuff;
    unsigned int *crcs = new unsigned int[shingleCount];
    for (unsigned int i = 0; i < shingleCount; i++){
        crcs[i] = Crc32(reinterpret_cast<const unsigned char*>(buff + words[i]), (words[i + min(DefaultValues::WORDS_EACH_SHINGLE, (unsigned int)wordCount)] - words[i])*sizeof(wchar_t));
    }
    if (shingleCount > DefaultValues::MAX_SHINGLE_PER_TEXT)
        for (unsigned int i = 0; i < m_count; i++){
            unsigned int minData = crcs[0],
                    minI = 0;
            for (unsigned int j = 1; j < shingleCount; j++)
                if (crcs[j] < minData){
                    minData = crcs[j];
                    minI = j;
                }
            crcs[minI] = -1;
            m_data[i] = minData;
        }
    else
        for (unsigned int i = 0; i < m_count; i++)
            m_data[i] = crcs[i];
    time_t a;
    time(&a);
    m_header.dateTime = *(localtime(&a));
    m_header.authorGroup_len = strlen(m_textData->authorGroup);
    m_header.authorName_len = strlen(m_textData->authorName);
    m_header.data_len = strlen(m_textData->streamData);
    m_header.textName_len = strlen(m_textData->name);
    m_header.type = m_textData->type;
    delete[] words;
    delete ptrtxt;
    delete[] crcs;
    delete[] buff;
}


Shingle::~Shingle(void)
{
    delete m_textData;
}

const unsigned int * Shingle::getData(){
    return this->m_data;
}

unsigned int Shingle::getCount(){
    return m_count;
}

const t__text & Shingle::getText(){
    return *(this->m_textData);
}


void Shingle::save(DataSrcAbstract *targetDataSource){
    try{
        targetDataSource->save(m_data, m_count, m_header, m_textData);
    }
    catch(...){
        //TODO exceptions processing
    }
}



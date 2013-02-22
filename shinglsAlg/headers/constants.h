#pragma once


namespace DePlaguarism{
	//Shingle.h
	const int MAX_SHINGLE_PER_TEXT = 120; // max Shingles in text (if more then 120 minimal)
	const int WORDS_EACH_SHINGLE = 4; // words in each Shingle
	const int MIN_WORD_LENGTH = 3; // min word length to be processed

	//ShingleApp.h
	const float THRESHOLD_TO_SAVE = 0.6; // maximum value from which new texts won`t be added to the database
	const unsigned int DOCUMENTS_IN_RESPONSE = 10;// maximum count of documents which will be responced to client

	const char HASH_DB_NAME[] = "hash.db";
	const char DOCS_DB_NAME[] = "docs.db";
	const char ENV_NAME[] = "./db";
	
	//logger mode const
	const bool LOG_EVERY_FCALL = true;
}
//mode constants
//#define MODE_DO_NOT_SAVE_RESULTS // database file will not be saved
#pragma once


namespace DePlaguarism{
	//Shingle.h
	const int maxShingleCount = 120; // max Shingles in text (if more then 120 minimal)
	const int wordsInShingle = 5; // words in each Shingle
	const int minWordLength = 3; // min word length to be processed

	//ShingleApp.h
	const float similarity = 0.25; // minimum value from which result will be sent to client
	const float addingMax = 0.6; // maximum value from which new texts won`t be added to the database

	const char HASH_DB_NAME[] = "hash.db";
	const char DOCS_DB_NAME[] = "docs.db";
	const char ENV_NAME[] = "./db";
	
	//logger mode const
	const bool LOG_EVERY_FCALL = true;
}
//mode constants 
//#define MODE_DO_NOT_SAVE_RESULTS // database file will not be saved
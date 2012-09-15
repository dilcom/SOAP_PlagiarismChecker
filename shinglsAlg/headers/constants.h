#pragma once


//Shingle.h
#define maxShingleCount 120 // max Shingles in text (if more then 120 minimal)
#define wordsInShingle 10 // words in each Shingle
#define minWordLength 3 // min word length to be processed

//ShingleApp.h
#define similarity 0.25 // minimum value from which result will be sent to client
#define addingMax 0.6 // maximum value from which new texts won`t be added to the database

//mode constants 
//#define MODE_DO_NOT_SAVE_RESULTS // database file will not be saved
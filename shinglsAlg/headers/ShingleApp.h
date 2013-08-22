#ifndef SHINGLE_APP_H
#define SHINGLE_APP_H

#include "../headers/Shingles.h"
#include "../headers/ShingleAppLogger.h"

using namespace std;
/*
main class. It provides receiving massages and its processing
*/
namespace DePlaguarism{		
#ifdef WIN32
    unsigned _stdcall process_queue(void *soap);///< win32 variant
#else
    void *process_queue(void *soap);///< variant for posix createThread
#endif
    int enqueue(SOAP_SOCKET);///< add socket to multithreading queue
    SOAP_SOCKET dequeue();///< pops socket from queue

    struct Pair{
        unsigned int docId;
        float similarity;
        Pair (unsigned int _docId, float _similarity){
            docId = _docId;
            similarity = _similarity;
        }
    };

    bool operator==(const Pair & left, const Pair & right);

    struct ClassComp { ///< it`s here to use it in sort
        bool operator() (const Pair & left, const Pair & right) const;
    };

    class ShingleApp :
            public shingleService
    {
    protected:
        ClassComp objectcomp; ///< compare obj for sort algorithm
        vector<Pair> appResult;
        void findSimilar(t__text * txt);  ///< function compares new text with others already in the base
        ShingleAppLogger * Log;  ///< logger object. Sends messages in several streams
        int shingleAlgorithm(t__text * txt, result *res); ///< compare two texts using algorithm based on shingles
        bool flagContinue;///< setting to false will make application to stop (only after accepting one more connection)
        bool mainEx;///< setting to true will make instance to close DB handlers and free memory allocated for them
        DePlaguarism::DataSrcAbstract * dataSource; ///< represents a db
    public:
        void loadDB();///< initializes dataSorces
        void closeDB();///< closes dataSorces
        void resetDB();
        void stop();///< sets flagContinue to false, stops the application then it accepts one more connection
        void setMain();///< sets mainEx to true, allows application to close DB handlers and free memory allocated for them
        void setChild();///< sets mainEx to false & inits documentCount with 0
        virtual int run(int port);
        string nowToStr(); ///< converts current date/time to string
        string ipToStr(); ///< converts current client`s ip to string
        ShingleAppLogger & log();///< getter for Log field
        ShingleApp();
        virtual ~ShingleApp();
        virtual	int CompareText(t__text * txt, result *res);///< main method which process incoming request
    };

    bool txtValid(t__text * a);

}

#endif

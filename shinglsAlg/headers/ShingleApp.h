#ifndef SHINGLE_APP_H
#define SHINGLE_APP_H

#include "../headers/Shingles.h"

/*
main class. It provides receiving massages and its processing
*/
namespace DePlagiarism{
    //! Infinite loop of serve requests.
    /*!
      Each of this functions runs in separate thread. It waits till where is a new free socket in queue.
      When socket is there, it takes it and serve request on it. And again waits for queue.
      \param soap is pointer to ShingleApp object which will be used to serve requests.
      \sa enqueue(), dequeue()
    */
#ifdef WIN32
    unsigned _stdcall process_queue(void *soap);
#else
    void *process_queue(void *soap);
#endif
    //! Puts socket into a queue.
    /*!
      \sa process_queue(), dequeue()
    */
    int enqueue(SOAP_SOCKET);
    //! Pops socket from queue.
    /*!
      \return popped socket
      \sa process_queue(), enqueue()
    */
    SOAP_SOCKET dequeue();

    //! Pair of document id and percent of similarity with the document being analyzed.
    struct Pair{
        unsigned int docId;
        float similarity;
        Pair (unsigned int _docId, float _similarity){
            docId = _docId;
            similarity = _similarity;
        }
    };
    bool operator==(const Pair & left, const Pair & right);
    //! It is here to use it in sort
    struct ClassComp {
        bool operator() (const Pair & left, const Pair & right) const;
    };

    //! Main class of application. Server itself.
    class ShingleApp :
            public shingleService
    {
    protected:
        ClassComp objectcomp; ///< Compare obj for sort algorithm.
        std::vector<Pair> m_appResult; ///< Application result after serving a request.
        void findSimilar(t__text * txt);  ///< Function compares new text with others already stored in the DB.
        log4cpp::Category * logger;  ///< Logger object
        int shingleAlgorithm(t__text * txt, result *res); ///< Search for Plagiarism in \param txt using algorithm based on shingles.
        bool m_flagContinue;///< Setting to false will stop the serve cycle.
        bool m_mainEx;///< Setting to true will make instance to close DB handlers and free memory allocated for them
        DataSrcAbstract * m_dataSource; ///< Represents a db
        bool validateText(t__text * a);
    public:
        char threadName[30]; ///< ThreadName for logger
        void loadDB();///< Initializes dataSorces
        void closeDB();///< Closes dataSorces
        void stop();///< Sets flagContinue to false, stops the application
        void setMain();///< Sets mainEx to true, allows application to close DB handlers and free memory allocated for them
        //! Marks object as child
        /*!
         * Sets mainEx to false
         * \param i child number
         */
        void setChild(int i);
        log4cpp::Category & getLogger(); ///< Getter for log field
        //! Runs cycle of serve.
        /*!
          Main client only accepts connection and puts tasks to queue. Slave objects take tasks and do all the work.
          Run initializes all the slaves and makes it to work. Being stopped run() waits till all the slaves finish.
          \param port specifies port which will be listened by app.
          \return 0 if everything OK
          \sa process_queue(), enqueue(), dequeue()
        */
        virtual int run(int port);
        std::string nowToStr(); ///< Converts current date/time to string
        std::string ipToStr(); ///< Converts current client`s ipv4 to string
        ShingleApp();
        virtual ~ShingleApp();
        virtual	int CompareText(t__text * txt, result *res);///< Main method of service which process incoming request
    };

}

#endif

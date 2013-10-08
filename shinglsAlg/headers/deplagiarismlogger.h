#ifndef DEPLAGIARISMLOGGER_H
#define DEPLAGIARISMLOGGER_H

#include <config.h>

namespace DePlagiarism {
    //! Singleton class.
        /*!
          We have access to its instance from anywhere in application.
          All constructors and operator= are private to forbid creation of extra instances.
          This class also stores information needed by loggers
        */    
    class DeplagiarismLogger
    {
    private:    
        DeplagiarismLogger();
        DeplagiarismLogger(const DeplagiarismLogger & src){}
        DeplagiarismLogger & operator=(const DeplagiarismLogger&){}
        log4cpp::PatternLayout * logLayout; ///< Layout contains pattern for output
        log4cpp::Appender * logFileAppender; ///< Appender to store log in file
        log4cpp::Appender * logConsoleAppender; ///< Appender to store log to console
        log4cpp::Category * logCategory; ///< Objct used to output values to appenders
        //! \brief Gives an access to the only instance of class.
        static DeplagiarismLogger & getInstance(){
            static DeplagiarismLogger INSTANCE;
            return INSTANCE;
        }
    public:
        //! \brief Returnes category field of class
        static log4cpp::Category * getLogger() {
            return getInstance().logCategory;
        }

        ~DeplagiarismLogger();
    };
    
    typedef DeplagiarismLogger Log;
}
/*EMERG  = 0, 
  FATAL  = 0,
  ALERT  = 100,
  CRIT   = 200,
  ERROR  = 300, - any errors
  WARN   = 400, 
  NOTICE = 500, - any inforamtion about current state (up\down\listening)
  INFO   = 600, 
  DEBUG  = 700, - any information about requests and time to handle them
 */

#endif // DEPLAGIARISMLOGGER_H

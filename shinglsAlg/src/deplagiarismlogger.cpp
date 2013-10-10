#include <deplagiarismlogger.h>

using namespace DePlagiarism;
using namespace log4cpp;

DeplagiarismLogger::DeplagiarismLogger()
{
    PatternLayout * consoleLogLayout = new log4cpp::PatternLayout();
    PatternLayout * fileLogLayout = new log4cpp::PatternLayout();
    try {
        consoleLogLayout->setConversionPattern("[%p] %d{%k:%M:%S} %m%n");
        fileLogLayout->setConversionPattern("[%p] %d{%d.%m.%Y %k:%M:%S.%l} %m%n");
        // example:
        // [ERROR] 10.01.2021 14:32:09.132 start MESSAGE MESSAGE MESSAGE end
    } catch(...){}
    logFileAppender = new FileAppender("DefaultAppender", DePlagiarism::DefaultValues::LOG_FILE_NAME);
    logFileAppender->setLayout(fileLogLayout);
    logConsoleAppender = new OstreamAppender("Console", &std::cout);
    logConsoleAppender->setLayout(consoleLogLayout);
    logCategory = &(Category::getInstance("Main_thread"));
    logCategory->setAdditivity(true);
    logCategory->setAppender(logConsoleAppender);
    logCategory->setAppender(logFileAppender);
    logCategory->setPriority(Priority::DEBUG);
}

DeplagiarismLogger::~DeplagiarismLogger() {
    logCategory->shutdown();
}


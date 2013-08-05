#ifndef DATASRCABSTRACT_H
#define DATASRCABSTRACT_H

#include <vector>
#include <string.h>
#include <memory>
#include "./DocHeader.h"

#ifdef _WIN32
    #include "../include/Win32/db_cxx.h"
#else
    #include "../include/UNIX/db_cxx.h"
#endif

namespace DePlaguarism{

    class DataSrcAbstract
    {
    public:
        std::string * error;
        virtual std::vector<unsigned int> * getIdsByHashes(const unsigned int * hashes, unsigned int count) = 0;
        virtual void save(const unsigned int * hashes, unsigned int count, DocHeader header, t__text * txt) = 0;
        virtual void getDocument(unsigned int docNumber, t__text **trgt, soap * parent) = 0;
        virtual ~DataSrcAbstract() {};
    };

}

#endif // DATASRCABSTRACT_H

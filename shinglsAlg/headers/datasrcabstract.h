#ifndef DATASRCABSTRACT_H
#define DATASRCABSTRACT_H

#include <vector>
#include <string.h>
#include <memory>
#ifdef _WIN32
    #include "../include/Win32/db_cxx.h"
#else
    #include "../include/UNIX/db_cxx.h"
#endif

namespace DePlaguarism{

    class PieceOfData{ ///< officer no bada chargin' me if creating peace of data
    private:
        char * value;
        size_t size;
    public:
        PieceOfData(const char * ivalue, size_t isize); ///< copies ivalue
        PieceOfData(const PieceOfData & src); ///< copies value, not only value pointer
        ~PieceOfData();
        PieceOfData()
            :value(NULL){}
        PieceOfData & operator = (const PieceOfData & other);
        char * getValue() const { return value; }
        size_t getSize() const { return size; }
    };

    class DataSrcAbstract
    {
    public:
        virtual std::vector<PieceOfData> * getValues(const PieceOfData & key) = 0;
        virtual std::vector<PieceOfData> * getValues(const std::vector<PieceOfData> & keys) = 0;
        virtual void saveValue(PieceOfData * key, PieceOfData * data) = 0;
    };

}

#endif // DATASRCABSTRACT_H

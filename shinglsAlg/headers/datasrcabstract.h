#ifndef DATASRCABSTRACT_H
#define DATASRCABSTRACT_H

#include <vector>
#include <string.h>
#include <memory>
#include "../include/UNIX/db_cxx.h"

/* PieceOfData and KeyValuePair does not copy constructor parametres!
 * You should not free memory after constucting an object!
 * It will be freed itself then container object is destructed.
 */
namespace Deplaguarism{

    struct PieceOfData{ ///< officer no bada chargin' me if creating peace of data
        char * value;
        size_t size;
        PieceOfData(char * ivalue, size_t isize): value(ivalue), size(isize){}
        PieceOfData(const PieceOfData & src); ///< copies value, not only value pointer
        ~PieceOfData();
        PieceOfData & operator = (const PieceOfData & other);
    };

    struct KeyValuePair{
        PieceOfData * key;
        PieceOfData * data;
        KeyValuePair(PieceOfData * ikey, PieceOfData * idata): key(ikey), data(idata){}
        ~KeyValuePair();
    };

    class DataSrcAbstract
    {
    public:
        virtual std::shared_ptr<std::vector<PieceOfData> > getValues(const PieceOfData & key) = 0;
        virtual std::shared_ptr<std::vector<PieceOfData> > getValues(const std::vector<PieceOfData> & keys) = 0;
        virtual std::shared_ptr<std::vector<PieceOfData> > getUniqueValues(const std::vector<PieceOfData> & keys) = 0;
        virtual void saveValue(const KeyValuePair & item) = 0;
        virtual void saveValues(const std::vector<KeyValuePair> & items) = 0;
    };

}

#endif // DATASRCABSTRACT_H

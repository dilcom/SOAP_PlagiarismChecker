#ifndef DATASRCABSTRACT_H
#define DATASRCABSTRACT_H

#include <array>

namespace Deplaguarism{

    struct PieceOfData{
        char * value;
        size_t size;
        char additionals; ///< optional field, for example it is used by getUniqueValues to store count of repeatings
        PieceOfData(char * ivalue, size_t isize);
    };

    struct KeyValuePair{
        PieceOfData * key;
        PieceOfData * data;
        KeyValuePair(PieceOfData * ikey, PieceOfData * idata);
        ~KeyValuePair();
    };

    class DataSrcAbstract
    {
    public:
        std::array<PieceOfData*> * getValues(PieceOfData * key);
        std::array<PieceOfData*> * getValues(std::array<PieceOfData*> * keys);
        std::array<PieceOfData*> * getUniqueValues(std::array<PieceOfData*> * keys);
        void saveValue(KeyValuePair * item);
        void saveValues(std::array<KeyValuePair*> * items);
    };

}

#endif // DATASRCABSTRACT_H

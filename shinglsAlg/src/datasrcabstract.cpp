#include "../headers/datasrcabstract.h"

using namespace DePlaguarism;

PieceOfData::PieceOfData(const char *ivalue, size_t isize){
    size = isize;
    value = new char[size];
    std::copy(ivalue, ivalue + size, value);
}

PieceOfData::PieceOfData(const PieceOfData &src){
    size = src.getSize();
    value = new char[size];
    const char * vl = src.getValue();
    std::copy(vl, vl + size, value);
}

PieceOfData::~PieceOfData(){
    delete[] value;
}


PieceOfData & PieceOfData::operator = (const PieceOfData & other){
    if (this != &other){
        size = other.getSize();
        char * new_value = new char[size];
        const char * vl = other.getValue();
        std::copy(vl, vl + size, new_value);
        if (value)
            delete[] value;
        value = new_value;
    }
    return *this;
}

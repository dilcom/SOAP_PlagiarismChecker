#include "../headers/datasrcabstract.h"

using namespace Deplaguarism;


PieceOfData::PieceOfData(const PieceOfData &src){
    value = new char[src.size];
    std::copy(src.value, src.value + src.size, value);
    size = src.size;
}

PieceOfData::~PieceOfData(){
    delete[] value;
}


KeyValuePair::~KeyValuePair(){
    delete key;
    delete data;
}

PieceOfData & PieceOfData::operator = (const PieceOfData & other){
    if (this != &other){

        char * new_value = new char[other.size];
        std::copy(other.value, other.value + other.size, new_value);

        // 2: освобождаем "старую" память
        delete [] value;

        // 3: присваиваем значения в "новой" памяти объекту
        value = new_value;
        size = other.size;
    }
    // по соглашению всегда возвращаем *this
    return *this;
}

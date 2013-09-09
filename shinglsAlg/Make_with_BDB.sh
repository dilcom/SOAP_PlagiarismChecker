cd ./projects/QMAKE/
qmake DEFINES+=BERKELEYDB
make clean
make
cp ./shingle_app ../../bin

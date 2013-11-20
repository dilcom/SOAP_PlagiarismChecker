SOAP_PlagiarismChecker
=================

HOWTO install

I. Application itself

1) git clone https://github.com/dilcom/SOAP_PlagiarismChecker.git

2) ./shinglsAlg/Make_with_BDB.sh or ./shinglsAlg/Make_with_redis.sh
   
Done!

II. Redis Cluster

1) git clone https://github.com/antirez/redis.git

2) cd ./redis/

3) make

4.0) configure it (readme here https://github.com/antirez/redis), enable cluster mode and choose a port

4.1) my config here http://db.tt/vRSLkKCJ - only 1 node, if you need more, copy it and replace 6379 -> any_port

4.2) build hiredis

  4.2.1) cd ./deps/hiredis
  
  4.2.2) make ; make install  ! or use checkinstall or something like that

5) ./src/redis-server configFileName.conf
    each node have its own *.conf file, I used at least 3 nodes (to let masters choose slaves on crashed masters place)

6) ./src/redis-trib.rb create ip.address.node1:port1 ip.address.node2:port2 ... ip.address.nodeN:portN 
   it creates cluster of N nodes, all of them must be running

Done!

DEPENDENCIES

qt-devel -> to build application with qmake
libconfig
libhireds -> if you use redis as data source
libdb -> if you use BDB as data source

Since 08102013 you need to install log4cpp:
http://log4cpp.sourceforge.net/

after installation do this with root:
cp /usr/local/lib64/liblog4cpp.so.5 /usr/lib64/

#!/bin/bash
tar -xvf libmnl-1.0.3.tar.bz2
cd libmnl-1.0.3
./configure
make
make install
cd ..

tar -xvf libnfnetlink-1.0.1.tar.bz2
cd libnfnetlink-1.0.1
./configure
make
make install
cd ..


tar -xvf libnetfilter_queue-1.0.2.tar.bz2
cd libnetfilter_queue-1.0.2
./configure PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
make
make install
cd ..

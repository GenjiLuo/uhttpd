#!/bin/bash

mkdir tmp
pushd tmp

# basic
echo "Install basic tools..."
sudo apt-get -y install build-essential wget cmake tcl8.5

# libmicrohttpd

echo "Install libmicrohttpd"
sudo apt-get -y install libmicrohttpd-dev

# libevent

echo "Install libevent"
sudo apt-get -y install libevent-dev

# libevhtp

echo "Install libevthp"
wget https://github.com/ellzey/libevhtp/archive/1.2.9.tar.gz
tar zxvf 1.2.9.tar.gz
cd libevhtp-1.2.9
mkdir build && cd build
cmake .. -DEVHTP_BUILD_SHARED:STRING=ON
make
sudo make install
sudo ldconfig

# hiredis

echo "Install hiredis"
wget https://github.com/redis/hiredis/archive/v0.13.3.tar.gz
tar zxvf v0.13.3.tar.gz
cd hiredis-0.13.3
make
sudo make install
sudo ldconfig

# redis

echo "Install redis"
wget http://download.redis.io/releases/redis-stable.tar.gz
tar xzf redis-stable.tar.gz
cd redis-stable
make
make test
sudo make install
./install_server.sh

sudo service redis_6379 start

popd
rm -fr tmp

echo "Setup done!"

#!/bin/bash

TOP=`pwd`

[ ! -d tmp ] && mkdir tmp

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
cd $(TOP)/tmp
wget https://github.com/ellzey/libevhtp/archive/1.2.9.tar.gz
tar zxvf 1.2.9.tar.gz
cd libevhtp-1.2.9
[ ! -d build ] && mkdir build
cd build
cmake .. -DEVHTP_BUILD_SHARED:STRING=ON
make
sudo make install
sudo ldconfig
cd ../..

# hiredis

echo "Install hiredis"
cd $(TOP)/tmp
wget https://github.com/redis/hiredis/archive/v0.13.3.tar.gz
tar zxvf v0.13.3.tar.gz
cd hiredis-0.13.3
make
sudo make install
sudo ldconfig
cd -

# redis

echo "Install redis"
cd $(TOP)/tmp
wget http://download.redis.io/releases/redis-stable.tar.gz
tar xzf redis-stable.tar.gz
cd redis-stable
make
make test
sudo make install
cd utils
./install_server.sh
sudo update-rc.d redis_6379 defaults
cd ../..

sudo service redis_6379 start

# memcached

sudo apt-get install memcached
sudo service memcached start

# libmemcached

wget https://launchpad.net/libmemcached/1.0/1.0.18/+download/libmemcached-1.0.18.tar.gz
tar zxvf libmemcached-1.0.18.tar.gz
cd libmemcached-1.0.18
./configure
make
sudo make install


# cleanup
cd $(TOP)
rm -fr tmp

echo "Setup done!"

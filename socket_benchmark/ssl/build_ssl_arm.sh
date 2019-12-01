#!/bin/sh
#****************************************************************#
# ScriptName: build_boost.sh
# Author: $SHTERM_REAL_USER@alibaba-inc.com
# Create Date: 2019-05-13 19:17
# Modify Author: $SHTERM_REAL_USER@alibaba-inc.com
# Modify Date: 2019-09-25 11:17
# Function: 
#***************************************************************#
set -x
echo $1
echo $2

OPENSSL_INCLUDE=/mnt/pangu/test/test_c++11/sock_benchmark/openssl-1.0.2d
BOOST_INCLUDE=/mnt/pangu/test/test_c++11/boost_1_70_0-install-arm

BOOST_LIB=/mnt/pangu/test/test_c++11/boost_1_70_0-install-arm
OPENSSL_LIB=/mnt/pangu/test/test_c++11/sock_benchmark/openssl-1.0.2d

g++ -Wl,--verbose -std=c++11 $1 -I${OPENSSL_INCLUDE}/include -I${BOOST_INCLUDE}/include -o $2 ${BOOST_LIB}/lib/libboost_system.a ${BOOST_LIB}/lib/libboost_filesystem.a ${OPENSSL_LIB}/lib/libssl.a ${OPENSSL_LIB}/lib/libcrypto.a -lpthread -ldl 


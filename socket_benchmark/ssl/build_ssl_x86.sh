#!/bin/sh
#****************************************************************#
# ScriptName: build_boost.sh
# Author: $SHTERM_REAL_USER@alibaba-inc.com
# Create Date: 2019-05-13 19:17
# Modify Author: $SHTERM_REAL_USER@alibaba-inc.com
# Modify Date: 2019-09-25 11:17
# Function: 
#***************************************************************#
#set -x
echo $1
echo $2

g++ -std=c++11 $1 -I/disk1/yutuo/mnt_docker/test/test_c++11/boost_1_70_0-install-x86/include /disk1/yutuo/mnt_docker/test/test_c++11/boost_1_70_0-install-x86/lib/libboost_system.a /disk1/yutuo/mnt_docker/test/test_c++11/boost_1_70_0-install-x86/lib/libboost_filesystem.a -lpthread -lssl -lcrypto -o $2 


#!/bin/sh
#****************************************************************#
# ScriptName: build_boost.sh
# Author: $SHTERM_REAL_USER@alibaba-inc.com
# Create Date: 2019-05-13 19:17
# Modify Author: $SHTERM_REAL_USER@alibaba-inc.com
# Modify Date: 2019-05-13 19:17
# Function: 
#***************************************************************#
#set -x
#/mnt/pangu/test/test_c++11/boost_1_70_0-install-arm/lib
echo $1
echo $2
g++ -std=c++11 $1 -I/mnt/pangu/test/test_c++11/boost_1_70_0-install-arm/include \
		/mnt/pangu/test/test_c++11/boost_1_70_0-install-arm/lib/libboost_system.a -lpthread -o $2 

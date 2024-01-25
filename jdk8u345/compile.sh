#!/usr/bin/env bash

###################################################
#
# file: compile.sh
#
# @Author:   Iacovos G. Kolokasis
# @Version:  07-03-2021 
# @email:    kolokasis@ics.forth.gr
#
# Compile JVM
#
###################################################

#CC=gcc-7.2.0
#CXX=g++-7.2.0
# Detect the platform
PLATFORM=""
TARGET_PLATFORM="aarch64"
# Default CC
CC=gcc
CXX=g++

function detect_platform()
{
	PLATFORM="$(uname -p)"
	# Conditional setting of CC based on platform
	#if [[ $PLATFORM == x86_64 ]]; then
	if [[ $PLATFORM != $TARGET_PLATFORM ]]; then
	    CC=$TARGET_PLATFORM-linux-gnu-gcc
	    CXX=$TARGET_PLATFORM-linux-gnu-g++
	    echo "Build platform's cpu arch($PLATFORM) differs from deployment's platform cpu arch($TARGET_PLATFORM), using cross compiler: $CC"
	    #CC=aarch64-linux-gnu-gcc
	    #CXX=aarch64-linux-gnu-g++
	    #echo "Detected x86_64 platform, using cross compiler: $CC"
	#elif [[ $PLATFORM == aarch64 ]]; then
	#    CC=gcc
	#    CXX=g++
	#    echo "Detected aarch64 platform, using default compiler: $CC"
	else
	    echo "Build and deployment platforms have the same cpu arch($PLATFORM), using default compiler: $CC"
	    #echo "Unknown platform: $PLATFORM, using default compiler: $CC"
	fi
}

function usage()
{
    echo
    echo "Usage:"
    echo -n "      $0 [option ...] [-h]"
    echo
    echo "Options:"
    echo "      -r  Build release without debug symbols"
    echo "      -d  Build with debug symbols"
    echo "      -c  Clean and make"
    echo "      -u  Update JVM in root directory"
    echo "      -h  Show usage"
    echo

    exit 1
}
  
# Compile without debug symbols
function release() 
{
  make dist-clean
  CC=$CC CXX=$CXX \
  bash ./configure \
    --with-jobs=32 \
    --disable-debug-symbols \
    --with-extra-cflags='-O3' \
    --with-extra-cxxflags='-O3' \
    --with-target-bits=64 \
    --with-extra-ldflags=-lregions
  intercept-build make
  cd ../ 
  compdb -p jdk8u345 list > compile_commands.json
  mv compile_commands.json jdk8u345
  cd - || exit
}

# Compile with debug symbols and assertions
function debug_symbols_on() 
{
  make dist-clean
  CC=$CC CXX=$CXX \
  bash ./configure \
    --with-debug-level=fastdebug \
    --with-native-debug-symbols=internal \
    --with-target-bits=64 \
    --with-jobs=32 \
    --with-extra-ldflags=-lregions
  intercept-build make
  cd ../ 
  compdb -p jdk8u345 list > compile_commands.json
  mv compile_commands.json jdk8u345
  cd - || exit
}

function clean_make()
{
  make clean
  make
}

export_env_vars()
{
	detect_platform
	local PROJECT_DIR="$(pwd)/../"

	#export JAVA_HOME="/usr/lib/jvm/java-1.8.0-openjdk"
	export JAVA_HOME=/spare/$(whoami)/openjdk/jdk8u402-b06
	### TeraHeap Allocator
	export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LIBRARY_PATH
	export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LD_LIBRARY_PATH                                                                                           
	export PATH=${PROJECT_DIR}/allocator/include/:$PATH
	export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$C_INCLUDE_PATH                                                                                         
	export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$CPLUS_INCLUDE_PATH
}

while getopts ":drcmh" opt
do
  case "${opt}" in
    r)
      export_env_vars
      release
      ;;
    d)
      export_env_vars
      debug_symbols_on
      ;;
    c)
      export_env_vars
      clean_make
      ;;
    m)
      export_env_vars
      make
      ;;
    h)
      usage
      ;;
    *)
      usage
      ;;
  esac
done


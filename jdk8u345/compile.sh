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

# Declare an associative array used for error handling
declare -A ERRORS

# Define the "error" values
ERRORS[INVALID_OPTION]=1
ERRORS[INVALID_ARG]=2
ERRORS[PROGRAMMING_ERROR]=3

#CC=gcc-7.4.0
#CXX=g++-7.4.0
# Detect the platform
PLATFORM=""
TARGET_PLATFORM="aarch64"
# Default CC
CC=/archive/users/$(whoami)/gcc-7.4.0/bin/gcc-7.4.0
CXX=/archive/users/$(whoami)/gcc-7.4.0/bin/g++-7.4.0

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
  	echo "Usage: $0 [options]"
  	echo "Options:"
    echo
    echo "      -r, --release  			Run configure and build a release version"
    echo "      -f, --fastdebug			Run configure and build a fastdebug version"
    echo "      -c, --clean-and-make	Run clean and make"
    echo "      -m,	--make				Run make"
    echo "      -h, --help				Display this help message and exit"
    echo

    return 0 2>/dev/null || exit 0
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
function fastdebug() 
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

function run_clean_make()
{
  	make clean
  	make
}

function run_make()
{
	make
}

export_env_vars()
{
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

detect_platform
#Default GCC 
OPTIONS=g:rfch
LONGOPTIONS=gcc-version:,help
PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")

# Check for errors in getopt
if [[ $? -ne 0 ]]; then
    return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
fi

# Evaluate the parsed options
eval set -- "$PARSED"

while true; do
    case "$1" in
        -g|--gcc-version)
            CC="$CC-$2"
            CXX="$CXX-$2"
            echo "CC = $CC"
            echo "CXX = $CXX"
            shift 2
            ;;
        -r|--release)
        	export_env_vars
      		release
      		shift
      		;;
        -f|--fastdebug)
        	export_env_vars
      		fastdebug
      		shift
      		;;
        -c|--clean-and-make)
      		export_env_vars
      		run_clean_make
      		shift
      		;;
        -m|--make)
      		export_env_vars
      		run_make
      		shift
      		;;
        -h|--help)
            usage
            return 0 2>/dev/null || exit 0  # This will return if sourced, and exit if run as a standalone script
            ;;
	    --)
            shift
            break
            ;;
         *)
            echo "Programming error"
            return ${ERRORS[PROGRAMMING_ERROR]} 2>/dev/null || exit ${ERRORS[PROGRAMMING_ERROR]}  # This will return if sourced, and exit if run as a standalone script
            ;;
    esac
done

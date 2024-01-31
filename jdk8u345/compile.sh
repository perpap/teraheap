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
ERROS[UNKNOWN_PLATFORM]=4

# Detect the build and target platforms
PLATFORM=""
TARGET_PLATFORM="aarch64"
# Default compilers
CC=gcc
CXX=g++

function detect_platform()
{
	PLATFORM="$(uname -p)"
	# Conditional setting of CC and CXX based on host and target platform
	if [[ $PLATFORM != $TARGET_PLATFORM ]]; then
	    CC=$TARGET_PLATFORM-linux-gnu-gcc
	    CXX=$TARGET_PLATFORM-linux-gnu-g++
	    echo "Build platform's cpu arch($PLATFORM) differs from target's platform cpu arch($TARGET_PLATFORM), using cross compilers: $CC, $CXX"
	else
	    TARGET_PLATFORM=$PLATFORM
	    echo "Build and target platforms have the same cpu arch($PLATFORM), using default compilers: $CC, $CXX"
	fi
}

function usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo
    echo "      -t, --target            Select the target platorm architecture for cross-compilation eg. aarch64, x86_64"
    echo "      -g, --gcc               Select an installed gcc version eg. 7.4.0"
    echo "      -r, --release           Run configure and build a release version"
    echo "      -f, --fastdebug         Run configure and build a fastdebug version"
    echo "      -c, --clean             Run clean and make"
    echo "      -m, --make              Run make"
    echo "      -h, --help              Display this help message and exit"
    echo
    echo "Examples:"
    echo
    echo "  ./compile.sh -r             Run configure and build a release version using default gcc"
    echo "  ./compile.sh --release      Run configure and build a release version using default gcc"
    echo "  ./compile.sh -g 7.4.0 -r    Run configure and build a release version using gcc-7.4.0"

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
    --with-extra-cxxflags='-std=c++11 -O3' \
    --with-target-bits=64 \
    --with-extra-ldflags=-lregions \
    --with-boot-jdk=$BOOT_JDK8
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
    --with-extra-cxxflags='-std=c++11' \
    --with-target-bits=64 \
    --with-jobs=32 \
    --with-extra-ldflags=-lregions \
    --with-boot-jdk=$BOOT_JDK8
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
    detect_platform
    if [[ $TARGET_PLATFORM == aarch64 ]]; then
        export JAVA_HOME="/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.312.b07-2.el8_5.aarch64/jre"
    elif [[ $TARGET_PLATFORM == x86_64 ]]; then
        export JAVA_HOME="/usr/lib/jvm/java-1.8.0-openjdk"
        #export JAVA_HOME=/spare/$(whoami)/openjdk/jdk8u402-b06
    else
        echo "Unknown platform"
        return ${ERRORS[UNKNOWN_PLATFORM]} 2>/dev/null || exit ${ERRORS[UNKNOWN_PLATFORM]}  # This will return if sourced, and exit if run as a standalone script
    fi
	echo "JAVA_HOME = $JAVA_HOME"
	
	### TeraHeap Allocator
    export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LIBRARY_PATH
    export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LD_LIBRARY_PATH                                                                                           
    export PATH=${PROJECT_DIR}/allocator/include/:$PATH
    export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$C_INCLUDE_PATH                                                                                         
    export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$CPLUS_INCLUDE_PATH
}

#Default GCC 
OPTIONS=t:g:rfcmh
LONGOPTIONS=target:,gcc:,release,fastdebug,clean,make,help
PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")

# Check for errors in getopt
if [[ $? -ne 0 ]]; then
    return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
fi

# Evaluate the parsed options
eval set -- "$PARSED"

while true; do
    case "$1" in
        -t|--target)
            TARGET_PLATFORM="$2"
            echo "TARGET_PLATFORM = $TARGET_PLATFORM"
            shift 2
            ;;
        -g|--gcc)
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
        -c|--clean)
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

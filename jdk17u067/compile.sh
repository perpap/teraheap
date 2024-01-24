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
	
PROJECT_DIR="$(pwd)/.."

# Declare an associative array used for error handling
declare -A ERRORS

# Define the "error" values
ERRORS[INVALID_OPTION]=1
ERRORS[INVALID_ARG]=2
ERRORS[MUTUALLY_EXCLUSIVE_OPTIONS]=3
ERRORS[OUT_OF_RANGE]=4
ERRORS[NOT_AN_INTEGER]=5
ERRORS[UNSUPPORTED_CIPHER]=6
ERRORS[PROGRAMMING_ERROR]=7

# Default values
BOOT_JDK_HOME_DEFAULT="$HOME"
OPENJDK_HOME_DEFAULT="$HOME"
EXPORT_TO_FILE=""
# Variables to track if the flags are set
SLOWDEBUG_SET=0
FASTDEBUG_SET=0
RELEASE_SET=0
ALL_SET=0
jvm_build="release"
openjdk_dir=$(pwd)

# Detect the platform
PLATFORM=""
TARGET_PLATFORM=""
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



: '
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
'
# Function to display usage message
function usage() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -t, --target-platform <cpu arch>	Set TARGET_PLATFORM to the cpu arch specified eg: x86_64, aarch64, ..." 
  echo "  -b, --boot-jdk <path>     		Set BOOT_JDK_HOME to the specified path."
  echo "  -j, --openjdk <path>      		Set OPENJDK_HOME to the specified path."
  echo "  -e, --export-to <path>   	 	Export environment variables to the specified file. If no path is provided, export to the current session."
  echo "  -f, --fastdebug           		Configure and build a fastdebug version."
  echo "  -r, --release             		Configure and build a release version."
  echo "  -a, --all                 		Configure and build both a fastdebug and a release version"
  echo "  -h, --help                		Display this help message and exit."
  return 0 2>/dev/null || exit 0
}
  
# Compile without debug symbols
function release() 
{
  make dist-clean
  CC=$CC CXX=$CXX \
  bash ./configure \
    --with-debug-level=release \
    --disable-warnings-as-errors \
    --enable-ccache \
    --with-jobs="$(nproc)" \
    --with-extra-cflags="-O3 -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-extra-cxxflags="-O3 -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-target-bits=64 \
    --with-boot-jdk=$BOOT_JDK
  
  intercept-build make CONF=linux-$TARGET_PLATFORM-server-release
  cd ../ 
  compdb -p jdk17u067 list > compile_commands.json
  mv compile_commands.json jdk17u067
  cd - || exit
}

# Compile with debug symbols and assertions
function debug_symbols_on() 
{
  make dist-clean
  CC=$CC CXX=$CXX \
  bash ./configure \
    --with-debug-level=fastdebug \
    --disable-warnings-as-errors \
    --with-native-debug-symbols=internal \
    --enable-ccache \
    --with-target-bits=64 \
    --with-jobs="$(nproc)" \
    --with-extra-cflags="-I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-extra-cxxflags="-I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-boot-jdk=$BOOT_JDK

  intercept-build make CONF=linux-$TARGET_PLATFORM-server-fastdebug
  cd ../ 
  compdb -p jdk17u067 list > compile_commands.json
  mv compile_commands.json jdk17u067
  cd - || exit
}

function clean_make()
{
  if [[ "$jvm_build" == "release" || "$jvm_build" == "r" ]]; then
    make CONF=linux-$TARGET_PLATFORM-server-release clean && make CONF=linux-$TARGET_PLATFORM-server-release distclean && make CONF=linux-$TARGET_PLATFORM-server-release images
  elif [ "$jvm_build" == "fastdebug" || "$jvm_build" == "f" ]; then  
    make CONF=linux-$TARGET_PLATFORM-server-fastdebug clean && make CONF=linux-$TARGET_PLATFORM-server-fastdebug distclean && make CONF=linux-$TARGET_PLATFORM-server-fastdebug images
  else
    echo "Mutliple configurations exist. Please provide a configuarion eg. release or fastdebug"
  fi
}

export_env_vars()
{
	detect_platform
	if [[ $PLATFORM == x86_64 && $TARGET_PLATFORM == aarch64 ]]; then
	  export BOOT_JDK=$DEPLOYMENT_AARCH64_BOOT_JDK
	fi
	#export JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
	export JAVA_HOME="/spare/perpap/openjdk/jdk-17.0.8.1+1"
	### TeraHeap Allocator
	export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib:$LIBRARY_PATH
	export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib:$LD_LIBRARY_PATH                                                                                           
	export PATH=${PROJECT_DIR}/allocator/include:$PATH
	export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include:$C_INCLUDE_PATH                                                                                         
	export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include:$CPLUS_INCLUDE_PATH
	export ALLOCATOR_HOME=${PROJECT_DIR}/allocator
	
    	export LIBRARY_PATH=${PROJECT_DIR}/tera_malloc/lib:$LIBRARY_PATH
	export LD_LIBRARY_PATH=${PROJECT_DIR}/tera_malloc/lib:$LD_LIBRARY_PATH                                                                                           
	export PATH=${PROJECT_DIR}/tera_malloc/include:$PATH
	export C_INCLUDE_PATH=${PROJECT_DIR}/tera_malloc/include:$C_INCLUDE_PATH                                                                                         
	export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/tera_malloc/include:$CPLUS_INCLUDE_PATH
	export TERA_MALLOC_HOME=${PROJECT_DIR}/tera_malloc
	
	#export LD_LIBRARY_PATH=/spare/miniconda3/envs/teraheap-aarch64-env/aarch64-conda-linux-gnu/sysroot/usr/lib64:$LD_LIBRARY_PATH
	echo "set LD_LIBRARY_PATH to '$LD_LIBRARY_PATH'"
}

OPTIONS=t:b:j:e:frah
LONGOPTIONS=target:,boot-jdk:,openjdk:,export-to:,fastdebug,release,all,help

# Use getopt to parse the options
PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")

# Check for errors in getopt
if [[ $? -ne 0 ]]; then
    return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
fi

# Evaluate the parsed options
eval set -- "$PARSED"

while true; do
    case "$1" in
    	-t|--target-platform)
    	    TARGET_PLATFORM="$2"
    	    shift 2
    	    ;;
        -b|--boot-jdk)
            BOOT_JDK_HOME="$2"
            shift 2
            ;;
        -j|--openjdk)
            OPENJDK_HOME="$2"
            shift 2
            ;;
        -e|--export-to)
            EXPORT_TO_FILE="$2"
            shift 2
            ;;
        -f|--fastdebug)
            FASTDEBUG_SET=1
            build="fastdebug"
            export_env_vars
            debug_symbols_on
            shift
            ;;
        -r|--release)
            RELEASE_SET=1	
            build="release"
            export_env_vars
      	    release
            shift
            ;;
        -a|--all)
            ALL_SET=1	
            build="all"
            shift
            ;;
        -h|--help)
            usage
            #break  # Exit the loop after displaying the usage message
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


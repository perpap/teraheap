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

# Detect the platform
PLATFORM="$(uname -p)"
# Default CC
CC=gcc

# Conditional setting of CC based on platform
if [[ $PLATFORM == x86_64 ]]; then
    CC=aarch64-linux-gnu-gcc
    echo "Detected x86_64 platform, using cross compiler: $CC"
elif [[ $PLATFORM == aarch64 ]]; then
    CC=gcc
    echo "Detected aarch64 platform, using default compiler: $CC"
else
    echo "Unknown platform: $PLATFORM, using default compiler: $CC"
fi

#CC=gcc-8
#CXX=g++-8
jvm_build="release"

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
    --with-debug-level=release \
    --disable-warnings-as-errors \
    --enable-ccache \
    --with-jobs="$(nproc)" \
    --with-extra-cflags="-O3 -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-extra-cxxflags="-O3 -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-target-bits=64 \
    --with-boot-jdk=$BOOT_JDK
  
  intercept-build make CONF=linux-x86_64-server-release
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

  intercept-build make CONF=linux-x86_64-server-fastdebug
  cd ../ 
  compdb -p jdk17u067 list > compile_commands.json
  mv compile_commands.json jdk17u067
  cd - || exit
}

function clean_make()
{
  if [[ "$jvm_build" == "release" ]]; then
    make CONF=linux-x86_64-server-release clean
  elif [ "$jvm_build" == "fx86_64astdebug" ]; then  
    make CONF=linux-x86_64-server-fastdebug clean
  else
    echo "Mutliple configurations exist. Please provide a configuarion eg. release or fastdebug"
  fi
}

export_env_vars()
{
	#export JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
	export JAVA_HOME="/spare/openjdk/jdk-17.0.8.1+1"
	### TeraHeap Allocator
	export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib:$LIBRARY_PATH
	export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib:$LD_LIBRARY_PATH                                                                                           
	export PATH=${PROJECT_DIR}/allocator/include:$PATH
	export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include:$C_INCLUDE_PATH                                                                                         
	export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include:$CPLUS_INCLUDE_PATH
	
    export LIBRARY_PATH=${PROJECT_DIR}/tera_malloc/lib:$LIBRARY_PATH
	export LD_LIBRARY_PATH=${PROJECT_DIR}/tera_malloc/lib:$LD_LIBRARY_PATH                                                                                           
	export PATH=${PROJECT_DIR}/tera_malloc/include:$PATH
	export C_INCLUDE_PATH=${PROJECT_DIR}/tera_malloc/include:$C_INCLUDE_PATH                                                                                         
	export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/tera_malloc/include:$CPLUS_INCLUDE_PATH
	
	export LD_LIBRARY_PATH=/spare/miniconda3/envs/teraheap-aarch64-env/aarch64-conda-linux-gnu/sysroot/usr/lib:$LD_LIBRARY_PATH
	echo "set LIBRARY_PATH to '$LD_LIBRARY_PATH'"
}

while getopts ":drcmh" opt
do
  case "${opt}" in
    r)
      export_env_vars
      release
      ;;
    d)
      build="fastdebug"
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


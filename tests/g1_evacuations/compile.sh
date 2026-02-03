#!/usr/bin/env bash

PROJECT_DIR="$(pwd)/../.."

export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LIBRARY_PATH
export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LD_LIBRARY_PATH
export PATH=${PROJECT_DIR}/allocator/include/:$PATH
export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$CPLUS_INCLUDE_PATH

ARCH=$(lscpu | grep "Architecture" | awk '{print $2}')
JAVAC=../../jdk17/build/linux-${ARCH}-server-release/jdk/bin/javac

# Clean everything
make --no-print-directory -C java distclean

# Firstly : make the wb.jar
cd ../Whitebox || exit 1
${JAVAC} -sourcepath . -d . jdk/test/**/**.java > /dev/null 2>&1
jar cf ./wb.jar . > /dev/null
find . -type f -name '*.class' -delete
cd - > /dev/null || exit 1

# Compile any benchmark you want
make --no-print-directory -C java all

#!/usr/bin/env bash

CLEAN_MODE="${1:-none}"
PROJECT_DIR="$(pwd)/../.."

export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LIBRARY_PATH
export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LD_LIBRARY_PATH
export PATH=${PROJECT_DIR}/allocator/include/:$PATH
export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$CPLUS_INCLUDE_PATH

if [[ "$CLEAN_MODE" == "clean" || "$CLEAN_MODE" == "distclean" ]]
then
  make --no-print-directory -C java ${CLEAN_MODE}
fi
make --no-print-directory -C java

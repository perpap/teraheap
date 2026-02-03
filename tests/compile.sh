#!/usr/bin/env bash

#-e: exit on command failure
#-u: error on unset variables 
#-o pipefail: pipelines fail if any command fails
set -euo pipefail

cd ./g1_evacuations || exit 1
./compile.sh
cd - > /dev/null || exit 1

cd ./g1_full_gc || exit 1
./compile.sh
cd - > /dev/null || exit 1

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
echo "PROJECT_DIR=$PROJECT_DIR"
# Declare an associative array used for error handling
declare -A ERRORS

# Define the "error" values
ERRORS[INVALID_OPTION]=1
ERRORS[INVALID_ARG]=2
ERRORS[PROGRAMMING_ERROR]=3
ERRORS[UNKNOWN_PLATFORM]=4

# Default values
BOOT_JDK_HOME_DEFAULT="$HOME"
BOOT_JDK=""
openjdk_dir=$(pwd)
DEBUG_SYMBOLS="none"
JVM_IMAGE_VARIANT=
BUILD_DIR=""
BUILD_ALL_JVM_VARIANTS=false
RELINK=false
CLEAN_AND_MAKE=false
# Detect the platform
PLATFORM=""
TARGET_PLATFORM="aarch64"
# Default CC
CC=gcc
CXX=g++

function detect_platform() {
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

# Function to display usage message
function usage() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -t, --target-platform <cpu arch>  Set TARGET_PLATFORM to the cpu arch specified eg: x86_64, aarch64, ..."
  echo "  -b, --boot-jdk <path>             Set BOOT_JDK to the specified path."
  echo "  -d, --build-directory <path>      Set build directory to the specified path."
  echo "  -g, --gcc                         Select an installed gcc version eg. 7.4.0"
  echo "  -i, --image <variant>             Configure and build a release|optimized|fastdebug|slowdebug image variant."
  echo "  -s, --debug-symbols <method>      Specify if and how native debug symbols should be built. Available methods are none, internal, external, zipped."
  echo "  -c, --clean                       Run clean and make"
  echo "  -m, --make                        Run make"
  echo "  -a, --all                         Configure and build all the jvm variants."
  echo "  -h, --help                        Display this help message and exit."
  echo
  echo "   Examples:"
  echo
  echo "  ./compile.sh -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -i \"release\"                            Configure and build a \"release\" image usigne ."
  echo "  ./compile.sh -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -i \"optimized\" -s \"internal\"          Configure and build an \"optimized\" image with \"internal\" debug symbols."
  echo "  ./compile.sh -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ --image \"fastdebug\"                     Configure and build a \"fastdebug\" image."
  echo "  ./compile.sh -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -g 7.4.0 -i \"release\"                   Configure and build a \"release\" image using gcc-7.4.0"
  echo "  ./compile.sh -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -g 13.2.0 -i \"release\"                  Configure and build a \"release\" image using gcc-13.2.0"
  echo "  ./compile.sh -m \"release\"                                                                         Relink a \"release\" image without running configure."
  return 0 2>/dev/null || exit 0
}
function build_jvm_image() {
  local image_variant=$1
  local debug_level=$1
  local debug_sumbols=$2

  if [[ $image_variant == "optimized" ]]; then
    image_variant="release"
  fi

  make CONF=linux-$TARGET_PLATFORM-server-$image_variant clean
  make CONF=linux-$TARGET_PLATFORM-server-$image_variant dist-clean

  bash ./configure \
    --with-debug-level=$debug_level \
    --with-native-debug-symbols=$debug_sumbols \
    --disable-warnings-as-errors \
    --enable-ccache \
    --with-jobs="$(nproc)" \
    --with-boot-jdk=$BOOT_JDK \
    --disable-cds \
    --with-extra-cflags="-march=armv8.2-a -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
    --with-extra-cxxflags="-march=armv8.2-a -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include"

  intercept-build make CONF=linux-$TARGET_PLATFORM-server-$image_variant
  cd ../
  compdb -p jdk17u067 list >compile_commands_$image_variant.json
  mv compile_commands_$image_variant.json jdk17u067
  cd - || exit
}

function build_all() {
  local debug_symbols=$1
  local image_variants=("release" "optimized" "fastdebug" "slowdebug")
  for image_variant in "${image_variants[@]}"; do
    if [[ $image_variant == "release" ]]; then
      build_jvm_image $image_variant "none"
    else
      if [[ $debug_sumbols == "none" ]]; then
        build_jvm_image $image_variant "internal"
      else
        build_jvm_image $image_variant $debug_sumbols
      fi
    fi
  done
}

function run_make() {
  if [[ $1 == "all" ]]; then
    local variants=("release" "fastdebug" "slowdebug")
    for variant in "${variants[@]}"; do
      intercept-build make CONF=linux-$TARGET_PLATFORM-server-$variant
      cd ../
      compdb -p jdk17u067 list >compile_commands_$variant.json
      mv compile_commands_$variant.json jdk17u067
      cd - || exit
    done
  else
    local variant=$1

    if [[ $variant == "optimized" ]]; then
      variant="release"
    fi
    intercept-build make CONF=linux-$TARGET_PLATFORM-server-$variant
    cd ../
    compdb -p jdk17u067 list >compile_commands_$variant.json
    mv compile_commands_$variant.json jdk17u067
    cd - || exit
  fi
}

function run_clean_make() {
  if [[ $1 == "all" ]]; then
    local variants=("release" "fastdebug" "slowdebug")
    for variant in "${variants[@]}"; do
      make CONF=linux-$TARGET_PLATFORM-server-$variant clean && make CONF=linux-$TARGET_PLATFORM-server-$variant dist-clean && make CONF=linux-$TARGET_PLATFORM-server-$variant images
    done
  else
    local variant=$1
    if [[ $variant == "optimized" ]]; then
      variant="release"
    fi
    make CONF=linux-$TARGET_PLATFORM-server-$variant clean && make CONF=linux-$TARGET_PLATFORM-server-$variant dist-clean && make CONF=linux-$TARGET_PLATFORM-server-$variant images
  fi
}

function export_env_vars() {
  #local PROJECT_DIR="$(pwd)/../"
  detect_platform

  export JAVA_HOME="/usr/lib/jvm/java-17-openjdk"
  #export JAVA_HOME="/spare/perpap/openjdk/jdk-17.0.8.1+1"
  echo "JAVA_HOME = $JAVA_HOME"

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

function parse_script_arguments() {
  OPTIONS=t:b:d:g:i:s:m:c:ah
  LONGOPTIONS=target:,boot-jdk:,build-directory,gcc:,image:,debug-symbols:,make:,clean:,all,help
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
    -t | --target-platform)
      TARGET_PLATFORM="$2"
      shift 2
      ;;
    -b | --boot-jdk)
      BOOT_JDK="$2"
      shift 2
      ;;
    -d | --build-directory)
      BUILD_DIR="$2"
      shift 2
      ;;
    -g | --gcc)
      CC="$CC-$2"
      CXX="$CXX-$2"
      echo "CC = $CC"
      echo "CXX = $CXX"
      shift 2
      ;;
    -i | --image)
      if [[ "$2" == "release" || "$2" == "r" || "$2" == "optimized" || "$2" == "o" || "$2" == "fastdebug" || "$2" == "f" || "$2" == "slowdebug" || "$2" == "s" ]]; then
        JVM_IMAGE_VARIANT="$2"
      else
        │ echo "Invalid jvm image variant; Use release|optimized|fastdebug|slowdebug or r|o|f|s"
        │ return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
      fi
      shift 2
      ;;
    -s | --debug-symbols)
      if [[ "$2" == "none" || "$2" == "internal" || "$2" == "external" || "$2" == "zipped" ]]; then
        DEBUG_SYMBOLS="$2"
      else
        │ echo "Invalid native debug symbols method; Use none|internal|extrenal|zipped"
        │ return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
      fi
      shift 2
      ;;
    -m | --make)
      if [[ "$2" == "all" || "$2" == "a" || "$2" == "release" || "$2" == "r" || "$2" == "optimized" || "$2" == "o" || "$2" == "fastdebug" || "$2" == "f" || "$2" == "slowdebug" || "$2" == "s" ]]; then
        RELINK=true
        JVM_IMAGE_VARIANT="$2"
        return 0 2>/dev/null || exit 0 # This will return if sourced, and exit if run as a standalone script
      else
        │ echo "Invalid jvm image variant; Please provide one of: all|release|optimized|fastdebug|slowdebug or a|r|o|f|s"
        │ return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
      fi
      shift 2
      ;;
    -c | --clean)
      if [[ "$2" == "all" || "$2" == "a" || "$2" == "release" || "$2" == "r" || "$2" == "optimized" || "$2" == "o" || "$2" == "fastdebug" || "$2" == "f" || "$2" == "slowdebug" || "$2" == "s" ]]; then
        CLEAN_AND_MAKE=true
        JVM_IMAGE_VARIANT="$2"
        return 0 2>/dev/null || exit 0 # This will return if sourced, and exit if run as a standalone script
      else
        │ echo "Invalid jvm image variant; Please provide one of: all|release|optimized|fastdebug|slowdebug or a|r|o|f|s"
        │ return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
      fi
      shift 2
      ;;
    -a | --all)
      BUILD_ALL_JVM_VARIANTS=true
      shift
      ;;
    -h | --help)
      usage
      #break  # Exit the loop after displaying the usage message
      return 0 2>/dev/null || exit 0 # This will return if sourced, and exit if run as a standalone script
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Programming error"
      return ${ERRORS[PROGRAMMING_ERROR]} 2>/dev/null || exit ${ERRORS[PROGRAMMING_ERROR]} # This will return if sourced, and exit if run as a standalone script
      ;;
    esac
  done
}

parse_script_arguments "$@"
export_env_vars
if [[ $RELINK == true ]]; then
  run_make $JVM_IMAGE_VARIANT
elif [[ $CLEAN_AND_MAKE == true ]]; then
  run_clean_make $JVM_IMAGE_VARIANT
elif [[ $BUILD_ALL_JVM_VARIANTS == true ]]; then
  build_all $DEBUG_SYMBOLS
else
  build_jvm_image $JVM_IMAGE_VARIANT $DEBUG_SYMBOLS
fi

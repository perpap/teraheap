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
ERRORS[PROGRAMMING_ERROR]=3
ERROS[UNKNOWN_PLATFORM]=4

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
TARGET_PLATFORM="aarch64"

# Default compilers
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

function usage() {
	echo "Usage: $0 [options]"
	echo "Options:"
	echo "  -t, --target-platform <cpu arch>	Set TARGET_PLATFORM to the cpu arch specified eg: x86_64, aarch64, ..."
	echo "  -b, --boot-jdk <path>     		Set BOOT_JDK_HOME to the specified path."
	echo "  -o, --openjdk <path>      		Set OPENJDK_HOME to the specified path."
	echo "  -g, --gcc                             Select an installed gcc version eg. 7.4.0"
	echo "  -r, --release                         Configure and build a release version."
	echo "  -f, --fastdebug           		Configure and build a fastdebug version."
	echo "  -c, --clean                           Run clean and make"
	echo "  -m, --make                            Run make"
	echo "  -a, --all                 		Configure and build both a fastdebug and a release version"
	echo "  -h, --help                		Display this help message and exit."
	echo
	echo "   Examples:"
	echo
	echo "  ./compile.sh -r                       Configure and build a release version using default gcc"
	echo "  ./compile.sh --release                Configure and build a release version using default gcc"
	echo "  ./compile.sh -g 7.4.0 -r              Configure and build a release version using gcc-7.4.0"
	echo "  ./compile.sh -g 13.2.0 -r             Configure and build a release version using gcc-13.2.0"

	return 0 2>/dev/null || exit 0
}

# Compile without debug symbols for ASPLOS'23 Artifact Evaluation
function artifact_evaluation() {
	make dist-clean
	#CC=$CC CXX=$CXX \
	bash ./configure \
		--with-jobs="$(nproc)" \
		--disable-debug-symbols \
		--with-extra-cflags='-O3 -march=native' \
		--with-extra-cxxflags='-O3 -march=native' \
		--with-extra-ldflags=-lregions \
		--with-boot-jdk=$BOOT_JDK8
	make
}

# Compile without debug symbols
function release() {
	: '
  #make dist-clean
  make CONF=linux-$TARGET_PLATFORM-normal-server-release clean
  make CONF=linux-$TARGET_PLATFORM-normal-server-release dist-clean
  #CC=$CC CXX=$CXX \
  bash ./configure \
    --with-debug-level=release \
    --disable-warnings-as-errors \
    --enable-ccache \
    --with-jobs=$NUM_CORES \
    --with-boot-jdk=$BOOT_JDK8 \
    --with-extra-cxxflags="-O3 -march=native" \
    --with-extra-ldflags=-lregions

  intercept-build make
  cd ../
  compdb -p jdk8u345 list > compile_commands.json
  mv compile_commands.json jdk8u345
  cd - || exit
  '
	#make dist-clean
	make CONF=linux-$TARGET_PLATFORM-normal-server-release clean
	make CONF=linux-$TARGET_PLATFORM-normal-server-release dist-clean

	#CC=$CC CXX=$CXX \
	bash ./configure \
		--with-debug-level=release \
		--enable-ccache \
		--with-jobs="$(nproc)" \
		--with-boot-jdk=$BOOT_JDK8 \
		--with-extra-cflags="-march=native -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
		--with-extra-cxxflags="-march=native -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
		--with-extra-ldflags=-lregions #--disable-cds \
	#--with-extra-cflags="-march=armv8.2-a -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
	#--with-extra-cxxflags="-march=armv8.2-a -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include"

	intercept-build make CONF=linux-$TARGET_PLATFORM-normal-server-release
	cd ../
	compdb -p jdk17u067 list >compile_commands_release.json
	mv compile_commands_release.json jdk17u067
	cd - || exit
}

# Compile with debug symbols and assertions
function fastdebug() {
	: '
  #make dist-clean
  make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug clean
  make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug dist-clean
  #CC=$CC CXX=$CXX \
  bash ./configure \
    --with-debug-level=fastdebug \
    --with-native-debug-symbols=internal \
    --enable-ccache \
    --with-jobs=$NUM_CORES \
    --with-boot-jdk=$BOOT_JDK8 \
    --with-extra-cxxflags="-O3 -march=native" \
    --with-extra-ldflags=-lregions

  intercept-build make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug
  cd ../
  compdb -p jdk8u345 list > compile_commands.json
  mv compile_commands.json jdk8u345
  cd - || exit
  '
	#make dist-clean
	make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug clean
	make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug dist-clean

	#CC=$CC CXX=$CXX \
	bash ./configure \
		--with-debug-level=fastdebug \
		--with-native-debug-symbols=internal \
		--enable-ccache \
		--with-jobs="$(nproc)" \
		--with-boot-jdk=$BOOT_JDK8 \
		--with-extra-cflags="-march=native -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
		--with-extra-cxxflags="-march=native -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
		--with-extra-ldflags=-lregions #--disable-cds \

	#--with-extra-cflags="-march=armv8.2-a -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include" \
	#--with-extra-cxxflags="-march=armv8.2-a -I${PROJECT_DIR}/allocator/include -I${PROJECT_DIR}/tera_malloc/include"

	intercept-build make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug
	cd ../
	compdb -p jdk8u345 list >compile_commands_fastdebug.json
	mv compile_commands_fastdebug.json jdk8u345
	cd - || exit
}

# Compile both with and without debug symbols
function release_and_fastdebug() {
	release
	fastdebug
}

function run_clean_make() {
	if [[ "$jvm_build" == "release" || "$jvm_build" == "r" ]]; then
		make CONF=linux-$TARGET_PLATFORM-normal-server-release clean && make CONF=linux-$TARGET_PLATFORM-normal-server-release dist-clean && make CONF=linux-$TARGET_PLATFORM-normal-server-release images
	elif [ "$jvm_build" == "fastdebug" || "$jvm_build" == "f" ]; then
		make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug clean && make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug dist-clean && make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug images
	else
		echo "Mutliple configurations exist. Please provide a configuarion eg. release or fastdebug"
	fi
}

function run_make() {
	: '
    intercept-build make CONF=linux-$TARGET_PLATFORM-normal-server-release
    cd ../
    compdb -p jdk8u345 list > compile_commands_release.json
    mv compile_commands_release.json jdk8u345
    cd - || exit
  '
	intercept-build make CONF=linux-$TARGET_PLATFORM-normal-server-release
	cd ../
	compdb -p jdk8u345 list >compile_commands_release.json
	mv compile_commands_release.json jdk8u345
	#cd - || exit
	cd -
	intercept-build make CONF=linux-$TARGET_PLATFORM-normal-server-fastdebug
	cd ../
	compdb -p jdk8u345 list >compile_commands_fastdebug.json
	mv compile_commands_fastdebug.json jdk8u345
	cd - || exit
}

export_env_vars() {
	local PROJECT_DIR="$(pwd)/../"
	detect_platform
	if [[ $TARGET_PLATFORM == aarch64 ]]; then
		export JAVA_HOME="/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.312.b07-2.el8_5.aarch64"
	elif [[ $TARGET_PLATFORM == x86_64 ]]; then
		export JAVA_HOME="/usr/lib/jvm/java-1.8.0-openjdk"
		#export JAVA_HOME=/spare/$(whoami)/openjdk/jdk8u402-b06
	else
		echo "Unknown platform"
		return ${ERRORS[UNKNOWN_PLATFORM]} 2>/dev/null || exit ${ERRORS[UNKNOWN_PLATFORM]} # This will return if sourced, and exit if run as a standalone script
	fi
	echo "JAVA_HOME = $JAVA_HOME"

	### TeraHeap Allocator
	export LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LIBRARY_PATH
	export LD_LIBRARY_PATH=${PROJECT_DIR}/allocator/lib/:$LD_LIBRARY_PATH
	export PATH=${PROJECT_DIR}/allocator/include/:$PATH
	export C_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$C_INCLUDE_PATH
	export CPLUS_INCLUDE_PATH=${PROJECT_DIR}/allocator/include/:$CPLUS_INCLUDE_PATH
}

OPTIONS=t:b:o:g:rfcmah
LONGOPTIONS=target:,boot-jdk:,openjdk:,gcc:,release,fastdebug,clean,make,all,help

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
		BOOT_JDK_HOME="$2"
		shift 2
		;;
	-o | --openjdk)
		OPENJDK_HOME="$2"
		shift 2
		;;
	-g | --gcc)
		CC="$CC-$2"
		CXX="$CXX-$2"
		echo "CC = $CC"
		echo "CXX = $CXX"
		shift 2
		;;
	-r | --release)
		RELEASE_SET=1
		build="release"
		export_env_vars
		release
		shift
		;;
	-f | --fastdebug)
		FASTDEBUG_SET=1
		build="fastdebug"
		export_env_vars
		fastdebug
		shift
		;;
	-c | --clean)
		export_env_vars
		run_clean_make
		shift
		;;
	-m | --make)
		export_env_vars
		run_make
		shift
		;;
	-a | --all)
		ALL_SET=1
		build="all"
		export_env_vars
		release_and_fastdebug
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

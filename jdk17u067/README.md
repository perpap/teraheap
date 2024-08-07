# Welcome to the JDK!

For build instructions please see the
[online documentation](https://openjdk.java.net/groups/build/doc/building.html),
or either of these files:

- [doc/building.html](doc/building.html) (html version)
- [doc/building.md](doc/building.md) (markdown version)

See <https://openjdk.java.net/> for more information about
the OpenJDK Community and the JDK.

To compile the JVM use the compile script.

# Compile Script Usage
This document provides usage instructions for the compile script.

### Description
The `usage` function displays the help message and usage instructions
for the compile script. This includes the available options and
examples of how to use the script.

### How to run the script
```sh
Usage: ./compile.sh [options]
Options:
  -t, --target-platform <cpu arch>  Set TARGET_PLATFORM to the cpu arch specified eg: x86_64, aarch64, ...
  -b, --boot-jdk <path>             Set BOOT_JDK to the specified path.
  -d, --build-directory <path>      Set build directory to the specified path.
  -g, --gcc                         Select an installed gcc version eg. 7.4.0
  -i, --image <variant>             Configure and build a release|optimized|fastdebug|slowdebug image variant.
  -s, --debug-symbols <method>      Specify if and how native debug symbols should be built. Available methods are none, internal, external, zipped.
  -c, --clean                       Run clean and make
  -m, --make                        Run make
  -a, --all                         Configure and build all the jvm variants.
  -h, --help                        Display this help message and exit.

   Examples:

  ./compile.sh -t x86_64 -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -i "release"                            Configure and build a "release" image usigne .
  ./compile.sh -t x86_64 -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -i "optimized" -s "internal"          Configure and build an "optimized" image with "internal" debug symbols.
  ./compile.sh -t x86_64 -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ --image "fastdebug"                     Configure and build a "fastdebug" image.
  ./compile.sh -t x86_64 -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -g 7.4.0 -i "release"                   Configure and build a "release" image using gcc-7.4.0
  ./compile.sh -t x86_64 -b /spare/perpap/openjdk/jdk16/jdk-16.0.2+7/ -g 13.2.0 -i "release"                  Configure and build a "release" image using gcc-13.2.0
  ./compile.sh -t x86_64 -m "release"
```

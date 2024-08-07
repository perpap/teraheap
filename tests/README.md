# TeraHeap Test Files

## Description
TeraHeap test files are used to test TeraHeap functionalities during
implementation. All these test files are implemented in JAVA. 

## Build
To build and run all test files for TeraHeap:

```sh
cd ./java
./compile.sh
cd -
```
## How to run the benchmarks

```sh
Usage:
      ./run.sh [option ...] [-h]
Options:
      -p, --point    <mount_point>        The mount point used for the H2 file(eg. /mnt/fmap/)
      -j, --jvm      <jvm_build>          The jvm build([release|r], [fastdebug|f], Default: release)
      -m, --mode     <execution_mode>     The jvm execution mode(0: Default, 1: Interpreter, 2: C1, 3: C2, 4: gdb, 5: ShowMessageBoxOnError)
      -t, --threads  <threads>            The number of GC threads (2, 4, 8, 16, 32)
      -f, --flexheap                      Enable flexheap
      -h  Show usage
```

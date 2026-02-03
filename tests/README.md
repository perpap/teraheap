# TeraHeap Test Files

g1_evacuations dir:
    tests for the g1 evacuations (forces minor and major evacuations)
g1_full_gc dir:
    tests for the g1 full gc cycle
                    
## Description
TeraHeap test files are used to test TeraHeap functionalities during
implementation. All these test files are implemented in JAVA. 

## Build
To build all test files testing G1 evacuation and full GC for TeraHeap:

```sh
./compile.sh
```
## Run Tests

```sh
Usage:
  ./run.sh -n <iterations> [-m <mode>] [-t <gc_threads_csv>] -d [<evac|full>] [-x "<jvm flags>"]... [-s <test1,test2,...>] [-b] [-h]

Options:
  -n <iterations>       Number of iterations to run.
  -m <mode>             Execution mode. One of: all, int, c1, c2, debug, msgbox.
                        (If omitted, the script should use its default mode which is all.)
  -t <gc_threads_csv>   Comma-separated list of Parallel GC thread counts (e.g., 1,2,4,8).
  -d <evac|full>        Select benchmark suite.
                        (If omitted, the script should run full GC benchmarks.)
  -x "<jvm flags>"      Extra JVM flags. You can pass multiple flags in one -x string
                        (space-separated), and you can also repeat -x multiple times.
                        Example: -x "-XX:+UnlockDiagnosticVMOptions -XX:+PrintCompilation" -x "-Dfoo=bar"
  -s <tests_csv>        Comma-separated list of specific tests/classes to run.
                        Example: -s ClassInstance,HashMap
  -b                    Stop the script on the first test failure.
  -h                    Show this help and exit.

Examples:
  ./run.sh -n 3 -m c2 -t 1,2,4 -d evac
  ./run.sh -n 1 -d full -b -s ClassInstance,TriggerImplicitGCs
  ./run.sh -n 5 -m int -d evac -x "-XX:+UnlockDiagnosticVMOptions -XX:+PrintInterpreter"

```

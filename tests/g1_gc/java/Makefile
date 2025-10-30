
###################################################
#
# file: Makefile
#
# @Author:   Iacovos G. Kolokasis
# @Version:  20-09-2020
# @email:    kolokasis@ics.forth.gr
#
# Makefile for test files
#
####################################################

JAVA_REPO := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))../../../jdk17
# JAVA_COMP=$(JAVA_REPO)/build/base-linux/jdk/bin/javac -Xlint:deprecation -sourcepath ../Whitebox/wb.jar -classpath ./bin/
JAVA_COMP=$(JAVA_REPO)/build/linux-x86_64-server-release/jdk/bin/javac -Xlint:deprecation -sourcepath ../Whitebox/wb.jar -classpath ./bin/
# JAVA_COMP=$(JAVA_REPO)/build/linux-x86_64-server-slowdebug/jdk/bin/javac -Xlint:deprecation -sourcepath ../Whitebox/wb.jar -classpath ./bin/


JAVAC=$(JAVA_COMP) -d ./bin

GDB=gdb

TARGET=GC.class Test_CM_WeakRef.class
	   
all: $(TARGET)

%.class: %.java
	$(JAVAC) $<

clean:
	rm -r out
	rm hotspot_pid*

distclean: clean
	rm -rf bin

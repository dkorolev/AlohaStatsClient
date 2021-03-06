.PHONY: test all indent clean

CPP = g++
CPPFLAGS = -std=c++11 -Wall -DBRICKS_JAVA
LDFLAGS =

# Option 1: Explicitly link against ../../src/bricks/java_wrapper/java_wrapper.cc.
# JNI_SRC = JNITest.cc  ../../src/bricks/net/api/test.cc  ../../src/bricks/java_wrapper/java_wrapper.cc

# Option 2: Use the configuration with a single C++ file and add -DLINK_JAVA_ON_LOAD_INTO_SOURCE
#           (In this particular example case, JNITest.cc will #include "test.cc" when this -D is set.)
CPPFLAGS += -DLINK_JAVA_ON_LOAD_INTO_SOURCE
JNI_SRC = JNITest.cc

JAVA_SRC = JNITest.java \
           ../../src/platform/android/src/main/java/org/alohastats/lib/Statistics.java \
           ../../src/platform/android/src/main/java/org/alohastats/lib/HttpTransport.java
JAR_MANIFEST = build/manifest
JAR = build/JNITest.jar
OS := $(shell uname)
ifeq ($(OS),Darwin)
  JNI_LIB = build/libalohastats.jnilib
  PATH_TO_SDK := $(shell xcodebuild -sdk macosx -version Path)
  CPPFLAGS += -I${PATH_TO_SDK}/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers
  LDFLAGS += -dynamiclib -o ${JNI_LIB}
else ifeq ($(OS),Linux)
  export SHELL := /bin/bash
  JNI_LIB = build/libalohastats.so
  JNI_PATH := $(shell readlink -f $(shell which javac))
  CPPFLAGS += -I$(dir ${JNI_PATH})../include -fPIC
  LDFLAGS += -o ${JNI_LIB} -shared
endif

test: all
	java -Djava.library.path=build -jar ${JAR}

all: build ${JNI_LIB} ${JAR}

${JAR_MANIFEST}:
	echo Main-Class: org.alohastats.lib.JNITest > ${JAR_MANIFEST}

${JAR}: build/java/org/alohastats/lib/JNITest.class ${JAR_MANIFEST}
	jar cvfm ${JAR} ${JAR_MANIFEST} -C build/java .

build/java/org/alohastats/lib/JNITest.class: ${JAVA_SRC}
	javac -d build/java ${JAVA_SRC}

indent:
	(find . -name "*.cc" ; find . -name "*.h") | xargs clang-format-3.5 -i

clean:
	rm -rf build

build: build/java
	mkdir -p $@

build/java:
	mkdir -p $@

${JNI_LIB}: ${JNI_SRC}
	${CPP} ${CPPFLAGS} ${JNI_SRC} ${LDFLAGS}

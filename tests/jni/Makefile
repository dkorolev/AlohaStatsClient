.PHONY: test all indent clean

CPP = clang++
CPPFLAGS = -std=c++11 -Wall
LDFLAGS =

JNI_SRC = ../../src/platform/android/src/main/jni/jni_wrapper.cc \
          ../http_client_test.cc \
          JNITest.cc
JAVA_SRC = @java.sources
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

build/java/org/alohastats/lib/JNITest.class:
	javac -d build/java @java.sources

indent:
	(find . -name "*.cc" ; find . -name "*.h") | xargs clang-format-3.5 -i

clean:
	rm -rf build

build: build/java
	mkdir -p $@

build/java:
	mkdir -p $@

${JNI_LIB}:
	${CPP} ${CPPFLAGS} ${JNI_SRC} ${LDFLAGS}

.PHONY: test all indent clean

CPP = clang++
CPPFLAGS = -std=c++11 -Wall
LDFLAGS =

JNI_SRC = ../../src/platform/android/src/main/jni/jni_wrapper.cc \
          ../http_client_test.cc \
          JNITest.cc
JNI_LIB = build/libalohastats.jnilib
JAVA_SRC = @java.sources
JAR_MANIFEST = build/manifest
JAR = JNITest.jar
OS := $(shell uname)
ifeq ($(OS),Darwin)
  PATH_TO_SDK := $(shell xcodebuild -sdk macosx -version Path)
  CPPFLAGS += -I${PATH_TO_SDK}/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers
  LDFLAGS += -dynamiclib -o ${JNI_LIB}
endif

test: all
	pushd build; java -jar ${JAR}; popd

all: build ${JNI_LIB} build/${JAR} #${BIN}

${JAR_MANIFEST}:
	echo Main-Class: org.alohastats.lib.JNITest > ${JAR_MANIFEST}

build/${JAR}: build/java/org/alohastats/lib/JNITest.class ${JAR_MANIFEST}
	jar cvfm build/${JAR} ${JAR_MANIFEST} -C build/java .

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

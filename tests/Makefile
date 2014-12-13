.PHONY: test all indent clean

CPP = clang++
CPPFLAGS = -std=c++11 -Wall
LDFLAGS = -pthread

SRC=../src/bricks/net/api/http_client_test.cc
BIN = build/test
OS := $(shell uname)
ifeq ($(OS),Darwin)
  CPPFLAGS += -x objective-c++ -fobjc-arc
  LDFLAGS += ../src/cpp/http_client_apple.mm -framework Foundation
endif

test: all
	${BIN}

all: build ${BIN}

indent:
	(find . -name "*.cc" ; find . -name "*.h") | xargs clang-format-3.5 -i

clean:
	rm -rf build

build:
	mkdir -p $@

${BIN}: ${SRC}
	${CPP} ${CPPFLAGS} -o $@ $< ${LDFLAGS}

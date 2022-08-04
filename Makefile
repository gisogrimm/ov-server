all: build binaries

export FULLVERSION:=$(shell cd libov && ./get_version.sh)

showver:
	echo $(VERSION)

BINARIES = ov-server

OBJ = ovtcpsocket

LIBOVOBJ = ov_types errmsg common udpsocket callerlist ov_tools MACAddressUtility

EXTERNALS = libcurl

BUILD_BINARIES = $(patsubst %,build/%,$(BINARIES))
BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))
BUILD_OBJ_LIBOV = $(patsubst %,build/%.o,$(LIBOVOBJ))

binaries: $(BUILD_OBJ_LIBOV) $(BUILD_OBJ)

CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++17 -pthread	\
-ggdb -fno-finite-math-only

CXXFLAGS += -DOVBOXVERSION="\"$(FULLVERSION)\""

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++17
PREFIX = /usr/local
BUILD_DIR = build
SOURCE_DIR = src

LDLIBS += -lcurl
#CXXFLAGS += -I/usr/include/x86_64-linux-gnu
LDLIBS += -ldl

#libov submodule:
CXXFLAGS += -Ilibov/src
#LDLIBS += -lovserver
#LDFLAGS += -Llibov/build

HEADER := $(wildcard src/*.h) $(wildcard libov/src/*.h)

OSFLAG :=
ifeq ($(OS),Windows_NT)
	OSFLAG += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OSFLAG += -D AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAG += -D IA32
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
		CXXFLAGS += -fext-numeric-literals
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
		LDFLAGS += -framework IOKit -framework CoreFoundation
	endif
		UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		OSFLAG += -D AMD64
	endif
		ifneq ($(filter %86,$(UNAME_P)),)
	OSFLAG += -D IA32
		endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		OSFLAG += -D ARM
	endif
endif

CXXFLAGS += $(OSFLAG)

lib: libov/Makefile
	$(MAKE) -C libov build libovserver

libov/Makefile:
	git submodule init
	git submodule update

build: build/.directory

%/.directory:
	mkdir -p $*
	touch $@

binaries: $(BUILD_BINARIES)

$(BUILD_BINARIES): libov/build/libovserver.a

build/%: src/%.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/%.o: src/%.cc $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%: libov/src/%.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/%.o: libov/src/%.cc $(HEADER)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~ ovclient*.deb
	$(MAKE) -C libov clean

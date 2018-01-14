CXXFLAGS	:=	-Wall -Wextra -pedantic -std=c++14 \
				-Wno-unused-result -Wno-unused-parameter \
				-O3 -g \
				-Ilibncgc/include -DNCGC_PLATFORM_TEST \
				$(CXXFLAGS)

ifdef UBSAN
CXXFLAGS	+=	-fsanitize=undefined
endif

LDFLAGS		:=	-Llibncgc/out/test -lncgc

# https://stackoverflow.com/questions/3774568/makefile-issue-smart-way-to-scan-directory-tree-for-c-files
rwildcard	=	$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRC_FILES	:=	src/main.cpp src/platform.cpp src/emulator.cpp \
				$(call rwildcard,src/emulators/,*.cpp) \
				$(call rwildcard,src/flashcart_core/,*.cpp)
OBJ_FILES	:=	$(patsubst %.cpp,%.o,$(SRC_FILES))

all: fcctester

fcctester: $(OBJ_FILES) libncgc/out/test/libncgc.a
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) $(LDFLAGS) -o $@

libncgc/out/test/libncgc.a:
	@$(MAKE) -C libncgc PLATFORM=test

clean:
	@rm -vf $(OBJ_FILES) fcctester
	@$(MAKE) -C libncgc clean

.PHONY: all clean

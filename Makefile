CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -Iinclude
LDFLAGS = -Llib -lvoicechat

UNAME_S := $(shell uname -s)
HOMEBREW_PREFIX := $(shell if [ -f /opt/homebrew/bin/brew ]; then echo /opt/homebrew; else echo /usr/local; fi)

# mac
ifeq ($(UNAME_S),Darwin)
    BREW_INCLUDES := $(addprefix -I$(HOMEBREW_PREFIX)/opt/, portaudio/include opus/include enet/include)
    BREW_LIBS := $(addprefix -L$(HOMEBREW_PREFIX)/opt/, portaudio/lib opus/lib enet/lib)
    
    CXXFLAGS += $(BREW_INCLUDES)
    LDFLAGS += $(BREW_LIBS)

# windows
else ifneq (,$(filter MINGW% CYGWIN% MSYS%,$(UNAME_S)))
    CXXFLAGS += -Iinclude
    LDFLAGS += -Llib -lws2_32 -lwinmm

# linux
else
    LDFLAGS += -L/usr/lib/x86_64-linux-gnu
endif

LDFLAGS += -lportaudio -lopus -lenet -lpthread

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
LIB_DIR = lib
BIN_DIR = bin

TARGET = $(BIN_DIR)/voice_chat
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
LIB_NAME = libvoicechat.a
LIB_TARGET = $(LIB_DIR)/$(LIB_NAME)

$(shell mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR))

all: $(TARGET)

$(TARGET): $(OBJS) $(LIB_TARGET)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_DIR)/main.cpp $(LDFLAGS) # doesn't this compite main twice?

$(LIB_TARGET): $(OBJS)
	ar rcs $@ $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

.PHONY: all clean
# 1. OS Detection
UNAME_S := $(shell uname -s)

# 2. Compiler and Flags
CXX ?= g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -Wno-deprecated-declarations

# 3. OS-Specific OpenSSL Paths
ifeq ($(UNAME_S), Darwin)
    # macOS Logic: Use Homebrew paths
    OPENSSL_PREFIX = $(shell brew --prefix openssl)
    INC = -I$(OPENSSL_PREFIX)/include
    LIB = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
else
    # Linux Logic: Use standard system paths
    INC = 
    LIB = -lssl -lcrypto
endif

# 4. Executable names
SENDER = sender
RECEIVER = receiver

# Default target
all: $(SENDER) $(RECEIVER)

$(SENDER): sender.cpp protocol.h
	$(CXX) $(CXXFLAGS) $(INC) sender.cpp -o $(SENDER) $(LIB)

$(RECEIVER): receiver.cpp protocol.h
	$(CXX) $(CXXFLAGS) $(INC) receiver.cpp -o $(RECEIVER) $(LIB)

clean:
	rm -f $(SENDER) $(RECEIVER)

.PHONY: all clean

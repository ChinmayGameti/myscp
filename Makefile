# 1. OS Detection
UNAME_S := $(shell uname -s)

# 2. Compiler and Flags
CXX ?= c++
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -Wno-deprecated-declarations

# 3. OS-Specific Paths for OpenSSL & libssh
ifeq ($(UNAME_S), Darwin)
    OPENSSL_PREFIX = $(shell brew --prefix openssl@3)
    LIBSSH_PREFIX = $(shell brew --prefix libssh)
    INC = -I$(OPENSSL_PREFIX)/include -I$(LIBSSH_PREFIX)/include
    LIB = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto -L$(LIBSSH_PREFIX)/lib -lssh
else
    INC = 
    LIB = -lssl -lcrypto -lssh
endif

# 4. Target Variables
SENDER = sender
RECEIVER = receiver
PREFIX = $(HOME)/.local/bin

all: $(SENDER) $(RECEIVER)

$(SENDER): sender.cpp common.h
	$(CXX) $(CXXFLAGS) $(INC) sender.cpp -o $(SENDER) $(LIB)

$(RECEIVER): receiver.cpp common.h
	$(CXX) $(CXXFLAGS) $(INC) receiver.cpp -o $(RECEIVER) $(LIB)

install: all
	@echo "Installing to $(PREFIX)..."
	mkdir -p $(PREFIX)
	cp $(SENDER) $(PREFIX)/myscp-send
	cp $(RECEIVER) $(PREFIX)/myscp-recv
	chmod +x $(PREFIX)/myscp-send
	chmod +x $(PREFIX)/myscp-recv
	@echo "Installation complete. Ensure $(PREFIX) is in your PATH."

uninstall:
	@echo "[UNINSTALL] Removing binaries from $(PREFIX)..."
	rm -f $(PREFIX)/myscp-send
	rm -f $(PREFIX)/myscp-recv
	@echo "myscp removed from system."

clean:
	@echo "[CLEAN] Removing compiled binaries..."
	rm -f $(SENDER) $(RECEIVER)
	@echo "[CLEAN] Removing object files and macOS debug symbols..."
	rm -rf *.o *.dSYM test/*.dSYM
	@echo "[CLEAN] Removing local test artifacts..."
	rm -f test/*.bin test/*.txt test/*.log test_payload.bin
	@echo "[CLEAN] Removing remote test artifacts (SSH loopback file)..."
	rm -f ~/test_payload.bin
	@echo "✨ Repository restored to pristine state."

.PHONY: all clean install uninstall

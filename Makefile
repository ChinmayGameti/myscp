# ==========================================
# MySCP Makefile
# ==========================================

UNAME_S := $(shell uname -s)
CXX ?= c++
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -Wno-deprecated-declarations

ifeq ($(UNAME_S), Darwin)
	OPENSSL_PREFIX = $(shell brew --prefix openssl@3)
	LIBSSH_PREFIX = $(shell brew --prefix libssh)
	INC = -I$(OPENSSL_PREFIX)/include -I$(LIBSSH_PREFIX)/include
	LIB = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto -L$(LIBSSH_PREFIX)/lib -lssh
else
	INC = 
	LIB = -lssl -lcrypto -lssh
endif

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
	@mkdir -p $(PREFIX)
	@cp $(SENDER) $(PREFIX)/myscp-send
	@cp $(RECEIVER) $(PREFIX)/myscp-recv
	@chmod +x $(PREFIX)/myscp-send
	@chmod +x $(PREFIX)/myscp-recv
	@echo "✅ Binaries installed to $(PREFIX)"
	@echo "------------------------------------------------------------"
	@if echo "$$PATH" | grep -q "$(PREFIX)"; then \
		echo "✅ $(PREFIX) is already in your PATH."; \
	else \
		echo "⚙️  Auto-configuring shell environment..."; \
		if [ "$$(basename $$SHELL)" = "zsh" ]; then \
			grep -qxF 'export PATH="$$PATH:$(PREFIX)"' ~/.zshrc || echo 'export PATH="$$PATH:$(PREFIX)"' >> ~/.zshrc; \
			echo "✅ Added $(PREFIX) to ~/.zshrc"; \
			echo "👉 Run 'source ~/.zshrc' or open a new terminal tab to use myscp."; \
		elif [ "$$(basename $$SHELL)" = "bash" ]; then \
			grep -qxF 'export PATH="$$PATH:$(PREFIX)"' ~/.bashrc || echo 'export PATH="$$PATH:$(PREFIX)"' >> ~/.bashrc; \
			echo "✅ Added $(PREFIX) to ~/.bashrc"; \
			echo "👉 Run 'source ~/.bashrc' or open a new terminal tab to use myscp."; \
		elif [ "$$(basename $$SHELL)" = "fish" ]; then \
			fish_add_path $(PREFIX); \
			echo "✅ Added $(PREFIX) to Fish paths."; \
		else \
			echo "⚠️  Could not auto-configure. Please add $(PREFIX) to your shell PATH manually."; \
		fi; \
	fi
	@echo "------------------------------------------------------------"

setup:
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		$(MAKE) setup-mac; \
	elif [ "$(UNAME_S)" = "Linux" ]; then \
		$(MAKE) setup-linux; \
	else \
		echo "❌ Unsupported OS: $(UNAME_S)"; \
	fi

setup-mac:
	@echo "🍎 macOS Detected: System-level SSH modifications are blocked by TCC."
	@echo "For local testing, run the SSH daemon in the foreground."
	@echo "------------------------------------------------------------------"
	@echo "👉 OPEN A NEW TERMINAL TAB AND RUN THIS EXACT COMMAND:"
	@echo "   sudo /usr/sbin/sshd -D -p 22"
	@echo "------------------------------------------------------------------"
	@echo "Keep that tab open. Then come back here to run myscp-send."

setup-linux:
	@echo "🐧 Linux Detected: Checking SSH configuration..."
	@if systemctl is-active --quiet ssh || systemctl is-active --quiet sshd; then \
		echo "✅ SSH daemon is already running."; \
	else \
		echo "⚠️  Attempting automated setup..."; \
		if command -v apt-get >/dev/null 2>&1; then \
			sudo apt-get update -qq && sudo apt-get install -y openssh-server >/dev/null 2>&1; \
			sudo systemctl enable --now ssh; \
		elif command -v dnf >/dev/null 2>&1; then \
			sudo dnf install -y openssh-server >/dev/null 2>&1; \
			sudo systemctl enable --now sshd; \
		elif command -v pacman >/dev/null 2>&1; then \
			sudo pacman -S --noconfirm openssh >/dev/null 2>&1; \
			sudo systemctl enable --now sshd; \
		fi; \
		echo "✅ Linux SSH daemon is now running!"; \
	fi

unit-test:
	@echo "Building unit tests..."
	$(CXX) $(CXXFLAGS) $(INC) test/unit_test.cpp -o test/runner $(LIB)
	@./test/runner

# Full end-to-end integration test suite
run-test: install
	@echo "Running integration test suite..."
	@bash test/run_test.sh
# ==========================================
# Maintenance
# ==========================================

uninstall:
	@echo "[UNINSTALL] Removing binaries from $(PREFIX)..."
	@rm -f $(PREFIX)/myscp-send
	@rm -f $(PREFIX)/myscp-recv
	@echo "✅ myscp removed from system."

clean:
	@echo "[CLEAN] Removing compiled binaries..."
	@rm -f $(SENDER) $(RECEIVER) test/runner
	@echo "[CLEAN] Removing object files and macOS debug symbols..."
	@rm -rf *.o *.dSYM test/*.dSYM
	@echo "[CLEAN] Destroying test sandbox and local payloads..."
	@rm -rf test/sandbox
	@rm -f test/*.bin test/*.txt test/*.log test_payload.bin payload.bin
	@echo "[CLEAN] Removing accidental home-directory payloads..."
	@rm -f ~/payload.bin ~/test_payload.bin
	@echo "✨ Repository restored to pristine state."
	@echo "💡 Note: This only cleans the local repository. To remove installed binaries from your system, run 'make uninstall'"

.PHONY: all install uninstall setup setup-mac setup-linux clean run-test unit-test
